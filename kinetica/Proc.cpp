#include "Proc.hpp"

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <iomanip>
#include <ios>
#include <sstream>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>

namespace
{
    template<typename T>
    std::string toString(const T& value)
    {
        std::ostringstream oss;
        oss << value;
        return oss.str();
    }

    std::string toHexString(const uint8_t* value, const std::size_t size)
    {
        std::ostringstream oss;
        oss << std::hex << std::setfill('0');

        for (std::size_t i = 0; i < size; ++i)
        {
            oss << std::setw(2) << (unsigned)value[i];
        }

        return oss.str();
    }

    std::string toIPv4String(const uint8_t* value)
    {
        char buffer[16];
        std::size_t length = sprintf(buffer, "%d.%d.%d.%d", (int)value[3], (int)value[2], (int)value[1], (int)value[0]);
        return std::string(buffer, length);
    }
}

namespace kinetica
{
    //--------------------------------------------------------------------------
    // MemoryMappedFile
    //--------------------------------------------------------------------------

    std::size_t ProcData::MemoryMappedFile::MEM_PAGE_SIZE = std::labs(sysconf(_SC_PAGESIZE));

    ProcData::MemoryMappedFile::MemoryMappedFile() :
        m_file(-1),
        m_writable(false),
        m_size(0),
        m_data(NULL),
        m_pos(0)
    {
    }

    ProcData::MemoryMappedFile::~MemoryMappedFile()
    {
        unmap();
    }

    void ProcData::MemoryMappedFile::map(const std::string& path, bool writable, std::size_t size)
    {
        unmap();
        m_file = open(path.c_str(), writable ? O_RDWR | O_CREAT : O_RDONLY, 0);

        if (m_file == -1)
        {
            throw std::runtime_error("Could not open map file: " + std::string(std::strerror(errno)));
        }

        m_writable = writable;
        remap(size);
    }

    void ProcData::MemoryMappedFile::remap(std::size_t size)
    {
        if (m_file == -1)
        {
            throw std::runtime_error("File not mapped");
        }

        if (size == (std::size_t)-1)
        {
            struct stat st;

            if (fstat(m_file, &st) != 0)
            {
                int err = errno;
                unmap();
                throw std::runtime_error("Could not get size of map file: " + std::string(std::strerror(err)));
            }

            size = st.st_size;
        }
        else if (m_writable)
        {
            if (ftruncate(m_file, size) != 0)
            {
                int err = errno;
                unmap();
                throw std::runtime_error("Could not set size of map file: " + std::string(std::strerror(err)));
            }
        }

        void* data;

        if (size == 0)
        {
            if (m_size > 0)
            {
                munmap(m_data, m_size);
                m_size = 0;
                m_data = NULL;
            }

            return;
        }
        else if (m_size == 0)
        {
            data = mmap(NULL, size, m_writable ? PROT_READ | PROT_WRITE : PROT_READ, MAP_SHARED, m_file, 0);
        }
        else
        {
            data = mremap(m_data, m_size, size, MREMAP_MAYMOVE);
        }

        if (data == MAP_FAILED)
        {
            int err = errno;
            unmap();
            throw std::runtime_error("Could not map file: " + std::string(std::strerror(err)));
        }

        m_data = data;
        m_size = size;
    }

    void ProcData::MemoryMappedFile::unmap()
    {
        if (m_file != -1)
        {
            if (m_size > 0)
            {
                munmap(m_data, m_size);
                m_size = 0;
                m_data = NULL;
            }

            close(m_file);
            m_file = -1;
            m_writable = false;
            m_pos = 0;
        }
    }

    std::size_t ProcData::MemoryMappedFile::getSize() const
    {
        return m_size;
    }

    std::size_t ProcData::MemoryMappedFile::getPos() const
    {
        return m_pos;
    }

    void ProcData::MemoryMappedFile::read(void *value, std::size_t length)
    {
        ensure(length);
        std::memcpy(value, &((char*)m_data)[m_pos], length);
        m_pos += length;
    }

    void ProcData::MemoryMappedFile::read(std::string& value)
    {
        uint64_t length = next<uint64_t>();
        ensure(length);
        value = std::string(&((char*)m_data)[m_pos], length);
        m_pos += length;
    }

    void ProcData::MemoryMappedFile::write(const void *value, const std::size_t length)
    {
        ensure(length);
        std::memcpy(&((char*)m_data)[m_pos], value, length);
        m_pos += length;
    }

    void ProcData::MemoryMappedFile::write(const std::string& value)
    {
        next<uint64_t>() = value.length();
        write(value.data(), value.length());
    }

    void ProcData::MemoryMappedFile::truncate()
    {
        remap(m_pos);
    }

    void ProcData::MemoryMappedFile::ensure(const std::size_t length)
    {
        if (m_pos + length > m_size)
        {
            if (!m_writable)
            {
                throw std::runtime_error("End of file reached");
            }
            else
            {
                std::size_t minSize = m_pos + length;
                remap(minSize + (MEM_PAGE_SIZE - (minSize % MEM_PAGE_SIZE)));
            }
        }
    }

    //--------------------------------------------------------------------------
    // Column
    //--------------------------------------------------------------------------

    std::size_t ProcData::Column::getTypeSize(const ColumnType type)
    {
        switch (type)
        {
            case BYTES:     return 8;
            case CHAR1:     return 1;
            case CHAR2:     return 2;
            case CHAR4:     return 4;
            case CHAR8:     return 8;
            case CHAR16:    return 16;
            case CHAR32:    return 32;
            case CHAR64:    return 64;
            case CHAR128:   return 128;
            case CHAR256:   return 256;
            case DATE:      return 4;
            case DECIMAL:   return 8;
            case DOUBLE:    return 8;
            case FLOAT:     return 4;
            case INT:       return 4;
            case INT8:      return 1;
            case INT16:     return 2;
            case IPV4:      return 4;
            case LONG:      return 8;
            case STRING:    return 8;
            case TIME:      return 4;
            case TIMESTAMP: return 8;
            default: throw std::runtime_error("Unknown data type: " + ::toString(type));
        }
    }

    ProcData::Column::Column(MemoryMappedFile& controlFile, bool writable)
    {
        controlFile.read(m_name);
        m_type = ColumnType(controlFile.next<uint64_t>());
        m_typeSize = getTypeSize(m_type);
        std::string dataPath;
        controlFile.read(dataPath);

        if (!dataPath.empty())
        {
            m_data.map(dataPath, writable);
            m_size = m_data.getSize() / m_typeSize;
        }
        else
        {
            m_size = 0;
        }

        std::string nullsPath;
        controlFile.read(nullsPath);

        if (!nullsPath.empty())
        {
            m_nulls.map(nullsPath, writable);
            m_isNullable = true;
        }
        else
        {
            m_isNullable = false;
        }

        std::string varDataPath;
        controlFile.read(varDataPath);

        if (!varDataPath.empty())
        {
            m_varData.map(varDataPath, writable);
        }
    }

    const std::string& ProcData::Column::getName() const
    {
        return m_name;
    }

    ProcData::Column::ColumnType ProcData::Column::getType() const
    {
        return m_type;
    }

    bool ProcData::Column::isNullable() const
    {
        return m_isNullable;
    }

    std::size_t ProcData::Column::getSize() const
    {
        return m_size;
    }

    const uint8_t* ProcData::Column::getNulls() const
    {
        return m_nulls.getData<uint8_t>();
    }

    bool ProcData::Column::isNull(const std::size_t index) const
    {
        if (!m_isNullable)
        {
            return false;
        }

        return m_nulls.getData<uint8_t>()[index];
    }

    std::vector<uint8_t> ProcData::Column::getVarBytes(const std::size_t index) const
    {
        const uint8_t* value = getVarValue<uint8_t>(index);
        return std::vector<uint8_t>(value, value + getVarValueSize<uint8_t>(index));
    }

    std::string ProcData::Column::getVarString(const std::size_t index) const
    {
        return std::string(getVarValue<char>(index), getVarValueSize<char>(index) - 1);
    }

    std::string ProcData::Column::toString(const std::size_t index) const
    {
        if (m_isNullable && isNull(index))
        {
            return "";
        }

        switch (m_type)
        {
            case BYTES: return ::toHexString(getVarValue<uint8_t>(index), getVarValueSize<uint8_t>(index));
            case CHAR1: return getValue<CharN<1> >(index);
            case CHAR2: return getValue<CharN<2> >(index);
            case CHAR4: return getValue<CharN<4> >(index);
            case CHAR8: return getValue<CharN<8> >(index);
            case CHAR16: return getValue<CharN<16> >(index);
            case CHAR32: return getValue<CharN<32> >(index);
            case CHAR64: return getValue<CharN<64> >(index);
            case CHAR128: return getValue<CharN<128> >(index);
            case CHAR256: return getValue<CharN<256> >(index);
            case DATE: return ::toString(getValue<int32_t>(index));
            case DECIMAL: return ::toString(getValue<int64_t>(index));
            case DOUBLE: return ::toString(getValue<double>(index));
            case FLOAT: return ::toString(getValue<float>(index));
            case INT: return ::toString(getValue<int32_t>(index));
            case INT8: return ::toString((int)getValue<int8_t>(index));
            case INT16: return ::toString(getValue<int16_t>(index));
            case IPV4: return ::toIPv4String(&getValue<uint8_t>(index * 4));
            case LONG: return ::toString(getValue<int64_t>(index));
            case STRING: return std::string(getVarValue<char>(index), getVarValueSize<char>(index) - 1);
            case TIME: return ::toString(getValue<int32_t>(index));
            case TIMESTAMP: return ::toString(getValue<int64_t>(index));
            default: throw std::runtime_error("Invalid data type");
        }
    }

    //--------------------------------------------------------------------------
    // InputColumn
    //--------------------------------------------------------------------------

    ProcData::InputColumn::InputColumn(MemoryMappedFile& controlFile) :
        Column(controlFile, false)
    {
    }

    //--------------------------------------------------------------------------
    // OutputColumn
    //--------------------------------------------------------------------------

    ProcData::OutputColumn::OutputColumn(MemoryMappedFile& controlFile) :
        Column(controlFile, true),
        m_pos(0)
    {
    }

    void ProcData::OutputColumn::setNull(const std::size_t index)
    {
        if (!m_isNullable)
        {
            throw std::logic_error("Column " + m_name + " is not nullable");
        }

        m_nulls.getData<uint8_t>()[index] = true;
    }

    std::size_t ProcData::OutputColumn::appendNull()
    {
        std::size_t index = m_pos;
        setNull(index);

        if (m_type == BYTES || m_type == STRING)
        {
            m_data.getData<uint64_t>()[index] = m_varData.getPos();
        }

        m_pos++;
        return index;
    }

    std::size_t ProcData::OutputColumn::appendVarBytes(const std::vector<uint8_t>& value)
    {
        return appendVarValue<uint8_t>(value.data(), value.size());
    }

    std::size_t ProcData::OutputColumn::appendVarString(const std::string& value)
    {
        return appendVarValue<char>(value.c_str(), value.length() + 1);
    }

    void ProcData::OutputColumn::complete()
    {
        if (m_type == BYTES || m_type == STRING)
        {
            m_varData.truncate();
        }
    }

    void ProcData::OutputColumn::reserve(const std::size_t size)
    {
        m_data.remap(size * getTypeSize(m_type));

        if (m_isNullable)
        {
            m_nulls.remap(size);
        }

        m_size = size;
    }

    //--------------------------------------------------------------------------
    // InputTable
    //--------------------------------------------------------------------------

    ProcData::InputTable::InputTable(MemoryMappedFile& controlFile) :
        Table(controlFile)
    {
    }

    //--------------------------------------------------------------------------
    // OutputTable
    //--------------------------------------------------------------------------

    ProcData::OutputTable::OutputTable(MemoryMappedFile& controlFile) :
        Table(controlFile)
    {
    }

    void ProcData::OutputTable::setSize(const std::size_t size)
    {
        for (std::size_t i = 0; i < m_columns.size(); ++i)
        {
            m_columns[i]->reserve(size);
        }

        m_size = size;
    }

    ProcData::OutputColumn& ProcData::OutputTable::getColumn(const std::size_t index)
    {
        if (index >= m_columns.size())
        {
            throw std::out_of_range("Column index out of range");
        }

        return *m_columns[index];
    }

    ProcData::OutputColumn& ProcData::OutputTable::operator[](const std::size_t index)
    {
        return getColumn(index);
    }

    ProcData::OutputColumn& ProcData::OutputTable::getColumn(const std::string& name)
    {
        std::map<std::string, OutputColumn*>::iterator column = m_columnMap.find(name);

        if (column != m_columnMap.end())
        {
            return *column->second;
        }
        else
        {
            throw std::out_of_range("Unknown column: " + name);
        }
    }

    ProcData::OutputColumn& ProcData::OutputTable::operator[](const std::string& name)
    {
        return getColumn(name);
    }

    void ProcData::OutputTable::complete()
    {
        for (std::size_t i = 0; i < m_columns.size(); ++i)
        {
            m_columns[i]->complete();
        }
    }

    //--------------------------------------------------------------------------
    // InputDataSet
    //--------------------------------------------------------------------------

    ProcData::InputDataSet::InputDataSet(MemoryMappedFile& controlFile) :
        DataSet(controlFile)
    {
    }

    //--------------------------------------------------------------------------
    // OutputDataSet
    //--------------------------------------------------------------------------

    ProcData::OutputDataSet::OutputDataSet(MemoryMappedFile& controlFile) :
        DataSet(controlFile)
    {
    }

    ProcData::OutputTable& ProcData::OutputDataSet::getTable(const std::size_t index)
    {
        if (index >= m_tables.size())
        {
            throw std::out_of_range("Table index out of range");
        }

        return *m_tables[index];
    }

    ProcData::OutputTable& ProcData::OutputDataSet::operator[](const std::size_t index)
    {
        return getTable(index);
    }

    ProcData::OutputTable& ProcData::OutputDataSet::getTable(const std::string& name)
    {
        typename std::map<std::string, OutputTable*>::iterator table = m_tableMap.find(name);

        if (table != m_tableMap.end())
        {
            return *table->second;
        }
        else
        {
            throw std::out_of_range("Unknown table: " + name);
        }
    }

    ProcData::OutputTable& ProcData::OutputDataSet::operator[](const std::string& name)
    {
        return getTable(name);
    }

    void ProcData::OutputDataSet::complete()
    {
        for (std::size_t i = 0; i < m_tables.size(); ++i)
        {
            m_tables[i]->complete();
        }
    }

    //--------------------------------------------------------------------------
    // ProcData
    //--------------------------------------------------------------------------

    ProcData ProcData::theProcData;

    ProcData* ProcData::get()
    {
        if (theProcData.m_requestInfo.empty())
        {
            theProcData.init();
        }

        return &theProcData;
    }

    ProcData::ProcData() :
        m_inputData(NULL),
        m_outputData(NULL)
    {
    }

    ProcData::~ProcData()
    {
        if (m_inputData)
        {
            delete m_inputData;
        }

        if (m_outputData)
        {
            delete m_outputData;
        }
    }

    void ProcData::init()
    {
        try
        {
            char* controlFileName = std::getenv("KINETICA_PCF");

            if (!controlFileName)
            {
                throw std::runtime_error("No control file specified");
            }

            MemoryMappedFile controlFile;
            controlFile.map(controlFileName, false);

            uint64_t version = controlFile.next<uint64_t>();

            if (version != 1)
            {
                throw std::runtime_error("Unrecognized control file version: " + toString(version));
            }

            controlFile.read(m_requestInfo);
            controlFile.read(m_requestInfo);
            controlFile.read(m_params);
            controlFile.read(m_binParams);
            m_inputData = new InputDataSet(controlFile);
            m_outputData = new OutputDataSet(controlFile);
            controlFile.read(m_outputControlFileName);
        }
        catch (...)
        {
            m_requestInfo.clear();
            m_params.clear();
            m_binParams.clear();

            if (m_inputData)
            {
                delete m_inputData;
                m_inputData = NULL;
            }

            if (m_outputData)
            {
                delete m_outputData;
                m_outputData = NULL;
            }

            throw;
        }
    }

    void ProcData::complete()
    {
        m_outputData->complete();
        MemoryMappedFile outputControlFile;
        outputControlFile.map(m_outputControlFileName, true);
        outputControlFile.next<uint64_t>() = 1;
        outputControlFile.write(m_results);
        outputControlFile.write(m_binResults);
    }

    #if __cplusplus > 199711L
    const std::map<std::string, std::string>& ProcData::getRequestInfo() const
    {
        return m_requestInfo;
    }

    const std::map<std::string, std::string>& ProcData::getParams() const
    {
        return m_params;
    }

    const std::map<std::string, std::vector<uint8_t> >& ProcData::getBinParams() const
    {
        return m_binParams;
    }
    #else
    std::map<std::string, std::string> ProcData::getRequestInfo() const
    {
        return m_requestInfo;
    }

    std::map<std::string, std::string> ProcData::getParams() const
    {
        return m_params;
    }

    std::map<std::string, std::vector<uint8_t> > ProcData::getBinParams() const
    {
        return m_binParams;
    }
    #endif

    const ProcData::InputDataSet& ProcData::getInputData() const
    {
        return *m_inputData;
    }

    std::map<std::string, std::string>& ProcData::getResults()
    {
        return m_results;
    }

    std::map<std::string, std::vector<uint8_t> >& ProcData::getBinResults()
    {
        return m_binResults;
    }

    ProcData::OutputDataSet& ProcData::getOutputData()
    {
        return *m_outputData;
    }
}
