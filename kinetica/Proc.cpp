#include "Proc.hpp"

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <iomanip>
#include <ios>
#include <sstream>
#include <unistd.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <sys/stat.h>

namespace
{
    const char* hexDigits = "0123456789abcdef";

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
    // Date
    //--------------------------------------------------------------------------

    Date::Date() :
        raw(0x8f821000) // 1000-01-01 (earliest allowed date)
    {
    }

    Date::Date(const unsigned year, const unsigned month, const unsigned day) :
        raw((((int)year - 1900) << 21)
            | (month << 17)
            | (day << 12))
    {
    }

    unsigned Date::getYear() const
    {
        return 1900 + (raw >> 21);
    }

    unsigned Date::getMonth() const
    {
        return (raw >> 17) & 0xf;
    }

    unsigned Date::getDay() const
    {
        return (raw >> 12) & 0x1f;
    }

    std::string Date::toString() const
    {
        std::ostringstream oss;
        oss << std::setfill('0')
            << std::setw(4) << getYear() << '-'
            << std::setw(2) << getMonth() << '-'
            << std::setw(2) << getDay();
        return oss.str();
    }

    bool Date::operator ==(const Date& value) const
    {
        return (raw & 0xFFFFF000) == (value.raw & 0xFFFFF000);
    }

    bool Date::operator !=(const Date& value) const
    {
        return (raw & 0xFFFFF000) != (value.raw & 0xFFFFF000);
    }

    bool Date::operator <(const Date& value) const
    {
        return (raw & 0xFFFFF000) < (value.raw & 0xFFFFF000);
    }

    bool Date::operator <=(const Date& value) const
    {
        return (raw & 0xFFFFF000) <= (value.raw & 0xFFFFF000);
    }

    bool Date::operator >(const Date& value) const
    {
        return (raw & 0xFFFFF000) > (value.raw & 0xFFFFF000);
    }

    bool Date::operator >=(const Date& value) const
    {
        return (raw & 0xFFFFF000) >= (value.raw & 0xFFFFF000);
    }

    std::ostream& operator <<(std::ostream& os, const Date& value)
    {
        os << value.toString();
        return os;
    }

    //--------------------------------------------------------------------------
    // DateTime
    //--------------------------------------------------------------------------

    DateTime::DateTime() :
        raw(0x8f82100000000000) // 1000-01-01 00:00:00.000 (earliest allowed datetime)
    {
    }

    DateTime::DateTime(const unsigned year, const unsigned month, const unsigned day,
                       const unsigned hour, const unsigned minute, const unsigned second, const unsigned millisecond) :
        raw((((long)year - 1900) << 53)
            | ((long)month << 49)
            | ((long)day << 44)
            | ((long)hour << 39)
            | ((long)minute << 33)
            | ((long)second << 27)
            | ((long)millisecond << 17))
    {
    }

    unsigned DateTime::getYear() const
    {
        return 1900 + (raw >> 53);
    }

    unsigned DateTime::getMonth() const
    {
        return (raw >> 49) & 0xf;
    }

    unsigned DateTime::getDay() const
    {
        return (raw >> 44) & 0x1f;
    }

    unsigned DateTime::getHour() const
    {
        return (raw >> 39) & 0x1f;
    }

    unsigned DateTime::getMinute() const
    {
        return (raw >> 33) & 0x3f;
    }

    unsigned DateTime::getSecond() const
    {
        return (raw >> 27) & 0x3f;
    }

    unsigned DateTime::getMillisecond() const
    {
        return (raw >> 17) & 0x3ff;
    }

    std::string DateTime::toString() const
    {
        std::ostringstream oss;
        oss << std::setfill('0')
            << std::setw(4) << getYear() << '-'
            << std::setw(2) << getMonth() << '-'
            << std::setw(2) << getDay() << ' '
            << std::setw(2) << getHour() << ':'
            << std::setw(2) << getMinute() << ':'
            << std::setw(2) << getSecond() << '.'
            << std::setw(3) << getMillisecond();
        return oss.str();
    }

    bool DateTime::operator ==(const DateTime& value) const
    {
        return (raw & 0xFFFFFFFFFFFE0000) == (value.raw & 0xFFFFFFFFFFFE0000);
    }

    bool DateTime::operator !=(const DateTime& value) const
    {
        return (raw & 0xFFFFFFFFFFFE0000) != (value.raw & 0xFFFFFFFFFFFE0000);
    }

    bool DateTime::operator <(const DateTime& value) const
    {
        return (raw & 0xFFFFFFFFFFFE0000) < (value.raw & 0xFFFFFFFFFFFE0000);
    }

    bool DateTime::operator <=(const DateTime& value) const
    {
        return (raw & 0xFFFFFFFFFFFE0000) <= (value.raw & 0xFFFFFFFFFFFE0000);
    }

    bool DateTime::operator >(const DateTime& value) const
    {
        return (raw & 0xFFFFFFFFFFFE0000) > (value.raw & 0xFFFFFFFFFFFE0000);
    }

    bool DateTime::operator >=(const DateTime& value) const
    {
        return (raw & 0xFFFFFFFFFFFE0000) >= (value.raw & 0xFFFFFFFFFFFE0000);
    }

    std::ostream& operator <<(std::ostream& os, const DateTime& value)
    {
        os << value.toString();
        return os;
    }

    //--------------------------------------------------------------------------
    // Time
    //--------------------------------------------------------------------------

    Time::Time() :
        raw(0)
    {
    }

    Time::Time(const unsigned hour, const unsigned minute, const unsigned second, const unsigned millisecond) :
        raw((hour << 26)
            | (minute << 20)
            | (second << 14)
            | (millisecond << 4))
    {
    }

    unsigned Time::getHour() const
    {
        return raw >> 26;
    }

    unsigned Time::getMinute() const
    {
        return (raw >> 20) & 0x3f;
    }

    unsigned Time::getSecond() const
    {
        return (raw >> 14) & 0x3f;
    }

    unsigned Time::getMillisecond() const
    {
        return (raw >> 4) & 0x3ff;
    }

    std::string Time::toString() const
    {
        std::ostringstream oss;
        oss << std::setfill('0')
            << std::setw(2) << getHour() << ':'
            << std::setw(2) << getMinute() << ':'
            << std::setw(2) << getSecond() << '.'
            << std::setw(3) << getMillisecond();
        return oss.str();
    }

    bool Time::operator ==(const Time& value) const
    {
        return raw == value.raw;
    }

    bool Time::operator !=(const Time& value) const
    {
        return raw != value.raw;
    }

    bool Time::operator <(const Time& value) const
    {
        return raw < value.raw;
    }

    bool Time::operator <=(const Time& value) const
    {
        return raw <= value.raw;
    }

    bool Time::operator >(const Time& value) const
    {
        return raw > value.raw;
    }

    bool Time::operator >=(const Time& value) const
    {
        return raw >= value.raw;
    }

    std::ostream& operator <<(std::ostream& os, const Time& value)
    {
        os << value.toString();
        return os;
    }

    //--------------------------------------------------------------------------
    // UUID
    //--------------------------------------------------------------------------

    UUID::UUID()
    {
        std::memset(raw, 0, 16);
    }

    std::string UUID::toString() const
    {
        std::string result;
        result.reserve(36);

        for (std::size_t i = 15; i >= 12; --i)
        {
            result.push_back(hexDigits[(raw[i] & 0xf0) >> 4]);
            result.push_back(hexDigits[raw[i] & 0x0f]);
        }

        result.push_back('-');

        for (std::size_t i = 11; i >= 10; --i)
        {
            result.push_back(hexDigits[(raw[i] & 0xf0) >> 4]);
            result.push_back(hexDigits[raw[i] & 0x0f]);
        }

        result.push_back('-');

        for (std::size_t i = 9; i >= 8; --i)
        {
            result.push_back(hexDigits[(raw[i] & 0xf0) >> 4]);
            result.push_back(hexDigits[raw[i] & 0x0f]);
        }

        result.push_back('-');

        for (std::size_t i = 7; i >= 6; --i)
        {
            result.push_back(hexDigits[(raw[i] & 0xf0) >> 4]);
            result.push_back(hexDigits[raw[i] & 0x0f]);
        }

        result.push_back('-');

        for (std::size_t i = 5; i != static_cast<std::size_t>(-1); --i)
        {
            result.push_back(hexDigits[(raw[i] & 0xf0) >> 4]);
            result.push_back(hexDigits[raw[i] & 0x0f]);
        }

        return result;
    }

    UUID& UUID::operator =(const UUID& value)
    {
        if (&value != this)
        {
            std::memcpy(raw, value.raw, 16);
        }

        return *this;
    }

    bool UUID::operator ==(const UUID& value) const
    {
        return std::memcmp(raw, value.raw, 16) == 0;
    }

    bool UUID::operator !=(const UUID& value) const
    {
        return std::memcmp(raw, value.raw, 16) != 0;
    }

    bool UUID::operator <(const UUID& value) const
    {
        for (std::size_t i = 0; i < 16; ++i)
        {
            if (raw[15 - i] == value.raw[15 - i])
            {
                continue;
            }
            else if (raw[15 - i] < value.raw[15 - i])
            {
                return true;
            }
            else
            {
                return false;
            }
        }

        return false;
    }

    bool UUID::operator <=(const UUID& value) const
    {
        for (std::size_t i = 0; i < 16; ++i)
        {
            if (raw[15 - i] == value.raw[15 - i])
            {
                continue;
            }
            else if (raw[15 - i] < value.raw[15 - i])
            {
                return true;
            }
            else
            {
                return false;
            }
        }

        return true;
    }

    bool UUID::operator >(const UUID& value) const
    {
        return !(*this <= value);
    }

    bool UUID::operator >=(const UUID& value) const
    {
        return !(*this < value);
    }

    uint8_t& UUID::operator [](std::size_t index)
    {
        return raw[15 - index];
    }

    const uint8_t& UUID::operator [](std::size_t index) const
    {
        return raw[15 - index];
    }

    std::ostream& operator <<(std::ostream& os, const UUID& value)
    {
        os << value.toString();
        return os;
    }

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

    bool ProcData::MemoryMappedFile::isMapped() const
    {
        return m_file != -1;
    }

    std::size_t ProcData::MemoryMappedFile::getSize() const
    {
        return m_size;
    }

    std::size_t ProcData::MemoryMappedFile::getPos() const
    {
        return m_pos;
    }

    void ProcData::MemoryMappedFile::seek(const std::size_t pos)
    {
        ensure(pos - m_pos);
        m_pos = pos;
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

    void ProcData::MemoryMappedFile::lock(const bool exclusive)
    {
        if (m_file == -1)
        {
            throw std::runtime_error("File not mapped");
        }

        int operation = exclusive ? LOCK_EX : LOCK_SH;

        int err;

        do
        {
            err = flock(m_file, operation);
        }
        while (err != 0 && errno == EINTR);

        if (err != 0)
        {
            throw std::runtime_error("Could not lock file: " + std::string(std::strerror(errno)));
        }
    }

    void ProcData::MemoryMappedFile::unlock()
    {
        if (m_file == -1)
        {
            return;
        }

        if (flock(m_file, LOCK_UN) != 0)
        {
            throw std::runtime_error("Could not unlock file: " + std::string(std::strerror(errno)));
        }
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
            case BOOLEAN:   return 1;
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
            case DATETIME:  return 8;
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
            case ULONG:     return 8;
            case UUID:      return 16;
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
            case BOOLEAN: return ::toString((int)getValue<int8_t>(index));
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
            case DATE: return getValue<Date>(index).toString();
            case DATETIME: return getValue<DateTime>(index).toString();
            case DECIMAL: return ::toString(getValue<int64_t>(index));
            case DOUBLE: return ::toString(getValue<double>(index));
            case FLOAT: return ::toString(getValue<float>(index));
            case INT: return ::toString(getValue<int32_t>(index));
            case INT8: return ::toString((int)getValue<int8_t>(index));
            case INT16: return ::toString(getValue<int16_t>(index));
            case IPV4: return ::toIPv4String(&getValue<uint8_t>(index * 4));
            case LONG: return ::toString(getValue<int64_t>(index));
            case STRING: return std::string(getVarValue<char>(index), getVarValueSize<char>(index) - 1);
            case TIME: return getValue<Time>(index).toString();
            case TIMESTAMP: return ::toString(getValue<int64_t>(index));
            case ULONG: return ::toString(getValue<uint64_t>(index));
            case UUID: return getValue<kinetica::UUID>(index).toString();
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
        Table<ProcData::InputColumn>(controlFile)
    {
    }

    //--------------------------------------------------------------------------
    // OutputTable
    //--------------------------------------------------------------------------

    ProcData::OutputTable::OutputTable(MemoryMappedFile& controlFile) :
        Table<ProcData::OutputColumn>(controlFile)
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
        DataSet<ProcData::InputTable>(controlFile)
    {
    }

    //--------------------------------------------------------------------------
    // OutputDataSet
    //--------------------------------------------------------------------------

    ProcData::OutputDataSet::OutputDataSet(MemoryMappedFile& controlFile) :
        DataSet<ProcData::OutputTable>(controlFile)
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
        std::map<std::string, OutputTable*>::iterator table = m_tableMap.find(name);

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

            if (version != 1 && version != 2)
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

            if (version == 2)
            {
                std::string statusFileName;
                controlFile.read(statusFileName);
                m_statusFile.map(statusFileName, true);
            }
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

    const std::string& ProcData::getStatus() const
    {
        return m_status;
    }

    void ProcData::setStatus(const std::string& value)
    {
        m_status = value;

        if (m_statusFile.isMapped())
        {
            m_statusFile.lock(true);

            try
            {
                m_statusFile.seek(0);
                m_statusFile.write(value);
            }
            catch (...)
            {
                m_statusFile.unlock();
                throw;
            }

            m_statusFile.unlock();
        }
    }
}
