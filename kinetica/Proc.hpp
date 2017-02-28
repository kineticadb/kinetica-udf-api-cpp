#ifndef _KINETICA_PROC_HPP_
#define _KINETICA_PROC_HPP_

#include <cstddef>
#include <cstring>
#include <map>
#include <ostream>
#include <stdexcept>
#include <stdint.h>
#include <string>
#include <vector>

namespace kinetica
{
    template<std::size_t N>
    struct CharN
    {
        char raw[N];

        CharN()
        {
            std::memset(raw, 0, N);
        }

        CharN(const char value)
        {
            raw[N - 1] = value;
            std::memset(raw, 0, N - 1);
        }

        CharN(const char* value)
        {
            std::size_t length = strlen(value);

            for (std::size_t i = 0; i < N; ++i)
            {
                if (i < length)
                {
                    raw[N - i - 1] = value[i];
                }
                else
                {
                    raw[N - i - 1] = 0;
                }
            }
        }

        CharN(const std::string& value)
        {
            for (std::size_t i = 0; i < N; ++i)
            {
                if (i < value.length())
                {
                    raw[N - i - 1] = value[i];
                }
                else
                {
                    raw[N - i - 1] = 0;
                }
            }
        }

        void operator =(const char value)
        {
            *this = CharN<N>(value);
        }

        void operator =(const char* value)
        {
            *this = CharN<N>(value);
        }

        void operator =(const std::string& value)
        {
            *this = CharN<N>(value);
        }

        char& operator[](std::size_t index)
        {
            return raw[N - index - 1];
        }

        const char& operator[](std::size_t index) const
        {
            return raw[N - index - 1];
        }

        operator std::string() const
        {
            std::string result;
            result.reserve(N);

            if (raw[0] != 0)
            {
                for (std::size_t i = 0; i < N; ++i)
                {
                    result.push_back(raw[N - i - 1]);
                }
            }
            else
            {
                for (std::size_t i = 0; i < N; ++i)
                {
                    if (raw[N - i - 1] == 0)
                    {
                        break;
                    }

                    result.push_back(raw[N - i - 1]);
                }
            }

            return result;
        }
    };

    template<std::size_t N>
    std::ostream& operator <<(std::ostream& os, const CharN<N>& value)
    {
        os << (std::string)value;
        return os;
    }


    class ProcData
    {
    private:
        class MemoryMappedFile
        {
        public:
            MemoryMappedFile();
            ~MemoryMappedFile();
            void map(const std::string& path, bool writable, std::size_t size = (std::size_t)-1);
            void remap(std::size_t size = (std::size_t)-1);
            void unmap();

            std::size_t getSize() const;

            template<typename T>
            T* getData()
            {
                return (T*)m_data;
            }

            template<typename T>
            const T* getData() const
            {
                return (T*)m_data;
            }

            std::size_t getPos() const;

            template<typename T>
            T& next()
            {
                ensure(sizeof(T));
                T* result = (T*)&((char*)m_data)[m_pos];
                m_pos += sizeof(T);
                return *result;
            }

            template<typename T>
            T* next(const std::size_t n)
            {
                ensure(sizeof(T) * n);
                T* result = (T*)&((char*)m_data)[m_pos];
                m_pos += sizeof(T);
                return result;
            }

            void read(void* value, const std::size_t length);
            void read(std::string& value);

            template<typename T>
            void read(std::vector<T>& value)
            {
                uint64_t length = next<uint64_t>();
                value.resize(length);
                read(value.data(), length * sizeof(T));
            }

            template<typename T>
            void read(std::map<std::string, T>& result)
            {
                uint64_t size = next<uint64_t>();

                while (size > 0)
                {
                    std::string key;
                    read(key);
                    read(result[key]);
                    size--;
                }
            }

            void write(const void* value, const std::size_t length);
            void write(const std::string& value);

            template<typename T>
            void write(const std::vector<T>& value)
            {
                next<uint64_t>() = value.size();
                write(value.data(), value.size() * sizeof(T));
            }

            template<typename T>
            void write(const std::map<std::string, T>& value)
            {
                next<uint64_t>() = value.size();

                for (typename std::map<std::string, T>::const_iterator entry = value.begin(); entry != value.end(); ++entry)
                {
                    write(entry->first);
                    write(entry->second);
                }
            }

            void truncate();

        private:
            static std::size_t MEM_PAGE_SIZE;

            int m_file;
            bool m_writable;
            std::size_t m_size;
            void* m_data;
            std::size_t m_pos;

            MemoryMappedFile(const MemoryMappedFile&);
            MemoryMappedFile& operator=(const MemoryMappedFile&);
            void ensure(const std::size_t length);
        };


    public:
        class Column
        {
        friend class ProcData;

        public:
            enum ColumnType
            {
                BYTES     = 0x0000002,
                CHAR1     = 0x0080000,
                CHAR2     = 0x0100000,
                CHAR4     = 0x0001000,
                CHAR8     = 0x0002000,
                CHAR16    = 0x0004000,
                CHAR32    = 0x0200000,
                CHAR64    = 0x0400000,
                CHAR128   = 0x0800000,
                CHAR256   = 0x1000000,
                DATE      = 0x2000000,
                DECIMAL   = 0x8000000,
                DOUBLE    = 0x0000010,
                FLOAT     = 0x0000020,
                INT       = 0x0000040,
                INT8      = 0x0020000,
                INT16     = 0x0040000,
                IPV4      = 0x0008000,
                LONG      = 0x0000080,
                STRING    = 0x0000001,
                TIME      = 0x4000000,
                TIMESTAMP = 0x0010000
            };

            static std::size_t getTypeSize(const ColumnType type);

            const std::string& getName() const;
            ColumnType getType() const;
            bool isNullable() const;
            std::size_t getSize() const;

            template<typename T>
            const T* getData() const
            {
                return m_data.getData<T>();
            }

            const uint8_t* getNulls() const;

            template<typename T>
            const T* getVarData() const
            {
                return m_varData.getData<T>();
            }

            bool isNull(const std::size_t index) const;

            template<typename T>
            const T& getValue(const std::size_t index) const
            {
                return m_data.getData<T>()[index];
            }

            template<typename T>
            const T* getVarValue(const std::size_t index) const
            {
                return &m_varData.getData<T>()[m_data.getData<uint64_t>()[index]];
            }

            template<typename T>
            std::size_t getVarValueSize(const std::size_t index) const
            {
                if (index < m_size - 1)
                {
                    return (m_data.getData<uint64_t>()[index + 1] - m_data.getData<uint64_t>()[index]) / sizeof(T);
                }
                else
                {
                    return (m_varData.getSize() - m_data.getData<uint64_t>()[index]) / sizeof(T);
                }
            }

            std::vector<uint8_t> getVarBytes(const std::size_t index) const;
            std::string getVarString(const std::size_t index) const;
            std::string toString(const std::size_t index) const;

        protected:
            std::string m_name;
            ColumnType m_type;
            std::size_t m_typeSize;
            bool m_isNullable;
            std::size_t m_size;
            MemoryMappedFile m_data;
            MemoryMappedFile m_nulls;
            MemoryMappedFile m_varData;

            Column(MemoryMappedFile& controlFile, bool writable);

        private:
            Column(const Column&);
            Column& operator=(const Column&);
        };


        class InputColumn : public Column
        {
        friend class ProcData;

        private:
            InputColumn(MemoryMappedFile& controlFile);
        };


        class OutputColumn : public Column
        {
        friend class ProcData;

        public:
            template<typename T>
            T* getData()
            {
                return m_data.getData<T>();
            }

            uint8_t* getNulls();

            template<typename T>
            T& getValue(const std::size_t index)
            {
                return m_data.getData<T>()[index];
            }

            template<typename T>
            T* getVarData()
            {
                return m_varData.getData<T>();
            }

            void setNull(const std::size_t index);
            std::size_t appendNull();

            template<typename T>
            void setValue(const std::size_t index, const T& value)
            {
                if (m_isNullable)
                {
                    m_nulls.getData<uint8_t>()[index] = false;
                }

                m_data.getData<T>()[index] = value;
            }

            template<typename T>
            std::size_t appendValue(const T& value)
            {
                std::size_t index = m_pos;
                setValue<T>(index, value);
                m_pos++;
                return index;
            }

            template<typename T>
            std::size_t appendVarValue(const T* value, const std::size_t size)
            {
                std::size_t index = m_pos;

                if (m_isNullable)
                {
                    m_nulls.getData<uint8_t>()[index] = false;
                }

                m_data.getData<uint64_t>()[index] = m_varData.getPos();
                m_varData.write(value, size * sizeof(T));
                m_pos++;
                return index;
            }

            std::size_t appendVarBytes(const std::vector<uint8_t>& value);
            std::size_t appendVarString(const std::string& value);

        private:
            std::size_t m_pos;

            OutputColumn(MemoryMappedFile& controlFile);
            void complete();
            void reserve(const std::size_t size);
        };


        template<typename T>
        class Table
        {
        friend class ProcData;

        public:
            const std::string getName() const
            {
                return m_name;
            }

            std::size_t getSize() const
            {
                return m_size;
            }

            std::size_t getColumnCount() const
            {
                return m_columns.size();
            }

            const T& getColumn(const std::size_t index) const
            {
                if (index >= m_columns.size())
                {
                    throw std::out_of_range("Column index out of range");
                }

                return *m_columns[index];
            }

            const T& operator[](const std::size_t index) const
            {
                return getColumn(index);
            }

            const T& getColumn(const std::string& name) const
            {
                typename std::map<std::string, T*>::const_iterator column = m_columnMap.find(name);

                if (column != m_columnMap.end())
                {
                    return *column->second;
                }
                else
                {
                    throw std::out_of_range("Unknown column: " + name);
                }
            }

            const T& operator[](const std::string& name) const
            {
                return getColumn(name);
            }

        protected:
            std::string m_name;
            std::size_t m_size;
            std::vector<T*> m_columns;
            std::map<std::string, T*> m_columnMap;

            Table(MemoryMappedFile& controlFile) :
                m_size(0)
            {
                controlFile.read(m_name);
                uint64_t columnCount = controlFile.next<uint64_t>();

                for (std::size_t i = 0; i < columnCount; ++i)
                {
                    T* column = new T(controlFile);

                    try
                    {
                        m_columns.push_back(column);
                    }
                    catch (...)
                    {
                        delete column;
                        throw;
                    }

                    m_columnMap[column->getName()] = &*column;

                    if (i == 0 || column->getSize() < m_size)
                    {
                        m_size = column->getSize();
                    }
                }
            }

            virtual ~Table()
            {
                for (std::size_t i = 0; i < m_columns.size(); ++i)
                {
                    delete m_columns[i];
                }
            }

        private:
            Table(const Table&);
            Table& operator=(const Table&);
        };


        class InputTable : public Table<InputColumn>
        {
        friend class ProcData;

        private:
            InputTable(MemoryMappedFile& controlFile);
        };


        class OutputTable : public Table<OutputColumn>
        {
        friend class ProcData;

        public:
            void setSize(const std::size_t size);
            OutputColumn& getColumn(const std::size_t index);
            OutputColumn& operator[](const std::size_t index);
            OutputColumn& getColumn(const std::string& name);
            OutputColumn& operator[](const std::string& name);

        private:
            OutputTable(MemoryMappedFile& controlFile);
            void complete();
        };


        template<typename T>
        class DataSet
        {
        public:
            std::size_t getTableCount() const
            {
                return m_tables.size();
            }

            const T& getTable(const std::size_t index) const
            {
                if (index >= m_tables.size())
                {
                    throw std::out_of_range("Table index out of range");
                }

                return *m_tables[index];
            }

            const T& operator[](const std::size_t index) const
            {
                return getTable(index);
            }

            const T& getTable(const std::string& name) const
            {
                typename std::map<std::string, T*>::const_iterator table = m_tableMap.find(name);

                if (table != m_tableMap.end())
                {
                    return *table->second;
                }
                else
                {
                    throw std::out_of_range("Unknown table: " + name);
                }
            }

            const T& operator[](const std::string& name) const
            {
                return getTable(name);
            }

        protected:
            std::vector<T*> m_tables;
            std::map<std::string, T*> m_tableMap;

            DataSet(MemoryMappedFile& controlFile)
            {
                uint64_t tableCount = controlFile.next<uint64_t>();

                for (std::size_t i = 0; i < tableCount; ++i)
                {
                    T* table = new T(controlFile);

                    try
                    {
                        m_tables.push_back(table);
                    }
                    catch (...)
                    {
                        delete table;
                        throw;
                    }

                    m_tableMap[table->getName()] = &*table;
                }
            }

            virtual ~DataSet()
            {
                for (std::size_t i = 0; i < m_tables.size(); ++i)
                {
                    delete m_tables[i];
                }
            }

        private:
            DataSet(const DataSet&);
            DataSet& operator=(const DataSet&);
        };


        class InputDataSet : public DataSet<InputTable>
        {
        friend class ProcData;

        private:
            InputDataSet(MemoryMappedFile& controlFile);
        };


        class OutputDataSet : public DataSet<OutputTable>
        {
        friend class ProcData;

        public:
            OutputTable& getTable(const std::size_t index);
            OutputTable& operator[](const std::size_t index);
            OutputTable& getTable(const std::string& name);
            OutputTable& operator[](const std::string& name);

        private:
            OutputDataSet(MemoryMappedFile& controlFile);
            void complete();
        };


        static ProcData* get();

        void complete();

        #if __cplusplus > 199711L
        const std::map<std::string, std::string>& getRequestInfo() const;
        const std::map<std::string, std::string>& getParams() const;
        const std::map<std::string, std::vector<uint8_t> >& getBinParams() const;
        #else
        std::map<std::string, std::string> getRequestInfo() const;
        std::map<std::string, std::string> getParams() const;
        std::map<std::string, std::vector<uint8_t> > getBinParams() const;
        #endif

        const InputDataSet& getInputData() const;

        std::map<std::string, std::string>& getResults();
        std::map<std::string, std::vector<uint8_t> >& getBinResults();
        OutputDataSet& getOutputData();

    private:
        static ProcData theProcData;

        std::map<std::string, std::string> m_requestInfo;
        std::map<std::string, std::string> m_params;
        std::map<std::string, std::vector<uint8_t> > m_binParams;
        InputDataSet* m_inputData;
        std::string m_outputControlFileName;
        std::map<std::string, std::string> m_results;
        std::map<std::string, std::vector<uint8_t> > m_binResults;
        OutputDataSet* m_outputData;

        ProcData();
        ProcData(const ProcData&);
        ~ProcData();
        ProcData& operator=(const ProcData&);
        void init();
    };
}

#endif
