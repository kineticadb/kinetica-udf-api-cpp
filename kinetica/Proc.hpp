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
    inline uint8_t swapBytes(const uint8_t value)
    {
        return value;
    }

    inline uint16_t swapBytes(const uint16_t value)
    {
        return ((value & 0xFF) << 8)
                | ((value & 0xFF00) >> 8);
    }

    inline uint32_t swapBytes(const uint32_t value)
    {
        return ((value & 0xFF) << 24)
                | ((value & 0xFF00) << 8)
                | ((value & 0xFF0000) >> 8)
                | ((value & 0xFF000000) >> 24);
    }

    inline uint64_t swapBytes(const uint64_t value)
    {
        return ((value & 0xFF) << 56)
                | ((value & 0xFF00) << 40)
                | ((value & 0xFF0000) << 24)
                | ((value & 0xFF000000) << 8)
                | ((value & 0xFF00000000) >> 8)
                | ((value & 0xFF0000000000) >> 24)
                | ((value & 0xFF000000000000) >> 40)
                | ((value & 0xFF00000000000000) >> 56);
    }

    template<typename T>
    struct CharNInt
    {
        static const std::size_t width = sizeof(T);

        union
        {
            T buffer;
            char raw[width];
        };

        void clear()
        {
            buffer = 0;
        }

        int compare(const CharNInt<T>& value) const
        {
            return buffer == value.buffer ? 0 :
                (buffer < value.buffer ? -1 : 1);
        }

        void set(const char value)
        {
            buffer = value << (8 * (sizeof(T) - 1));
        }

        void set(const char* value, const std::size_t length)
        {
            if (length >= width)
            {
                buffer = swapBytes(*(T*)value);
            }
            else
            {
                buffer = 0;

                for (std::size_t i = 0; i < length; ++i)
                {
                    raw[width - 1 - i] = value[i];
                }
            }
        }

        bool operator ==(const CharNInt<T>& value) const
        {
            return buffer == value.buffer;
        }

        bool operator !=(const CharNInt<T>& value) const
        {
            return buffer != value.buffer;
        }

        operator std::string() const
        {
            CharNInt<T> swapBuffer;
            swapBuffer.buffer = swapBytes(buffer);
            return raw[0] == 0 ? std::string(swapBuffer.raw) : std::string(swapBuffer.raw, width);
        }
    };

    template<>
    inline void CharNInt<uint8_t>::set(const char* value, const std::size_t length)
    {
        buffer = length == 0 ? 0 : (uint8_t)*value;
    }

    template<>
    inline CharNInt<uint8_t>::operator std::string() const
    {
        return buffer == 0 ? std::string() : std::string(raw, 1);
    }

    template<typename T, std::size_t N>
    struct CharNIntBuffer
    {
        static const std::size_t width = sizeof(T) * N;

        union
        {
            T buffer[N];
            char raw[width];
        };

        void clear()
        {
            std::memset(this, 0, width);
        }

        int compare(const CharNIntBuffer<T, N>& value) const
        {
            for (std::size_t i = 0; i < N; ++i)
            {
                if (buffer[N - 1 - i] == value.buffer[N - 1 - i])
                {
                    continue;
                }

                return buffer[N - 1 - i] < value.buffer[N - 1 - i] ? -1 : 1;
            }

            return 0;
        }

        void set(const char value)
        {
            std::memset(this, 0, width);
            raw[width - 1] = value;
        }

        void set(const char* value, const std::size_t length)
        {
            std::size_t i = 0;
            std::size_t pos = 0;

            if (length >= sizeof(T))
            {
                std::size_t limit = length - sizeof(T);

                for (; i < N && pos <= limit; ++i, pos += sizeof(T))
                {
                    buffer[N - 1 - i] = swapBytes(((T*)value)[i]);
                }

                if (i == N)
                {
                    return;
                }
            }

            if (length > pos)
            {
                std::size_t limit = ++i * sizeof(T);

                for (; pos < length && pos < limit; ++pos)
                {
                    raw[width - 1 - pos] = value[pos];
                }

                for (; pos < limit; ++pos)
                {
                    raw[width - 1 - pos] = 0;
                }
            }

            for (; i < N; ++i)
            {
                buffer[N - 1 - i] = 0;
            }
        }

        bool operator ==(const CharNIntBuffer<T, N>& value) const
        {
            return std::memcmp(this, &value, width) == 0;
        }

        bool operator !=(const CharNIntBuffer<T, N>& value) const
        {
            return std::memcmp(this, &value, width) != 0;
        }

        operator std::string() const
        {
            CharNIntBuffer<T, N> swapBuffer;

            for (std::size_t i = 0; i < N; ++i)
            {
                swapBuffer.buffer[i] = swapBytes(buffer[N - 1 - i]);

                if (swapBuffer.buffer[i] == 0)
                {
                    return std::string(swapBuffer.raw);
                }
            }

            return raw[0] == 0 ? std::string(swapBuffer.raw) : std::string(swapBuffer.raw, width);
        }
    };

    template<std::size_t N> struct CharNBase : public CharNIntBuffer<uint8_t, N> {};
    template<> struct CharNBase<1> : public CharNInt<uint8_t> {};
    template<> struct CharNBase<2> : public CharNInt<uint16_t> {};
    template<> struct CharNBase<4> : public CharNInt<uint32_t> {};
    template<> struct CharNBase<8> : public CharNInt<uint64_t> {};
    template<> struct CharNBase<16> : public CharNIntBuffer<uint64_t, 2> {};
    template<> struct CharNBase<32> : public CharNIntBuffer<uint64_t, 4> {};
    template<> struct CharNBase<64> : public CharNIntBuffer<uint64_t, 8> {};
    template<> struct CharNBase<128> : public CharNIntBuffer<uint64_t, 16> {};
    template<> struct CharNBase<256> : public CharNIntBuffer<uint64_t, 32> {};

    template<std::size_t N>
    struct CharN : CharNBase<N>
    {
        CharN()
        {
            this->clear();
        }

        CharN(const char value)
        {
            this->set(value);
        }

        CharN(const char* value)
        {
            this->set(value, strlen(value));
        }

        CharN(const std::string& value)
        {
            this->set(value.data(), value.length());
        }

        void operator =(const char value)
        {
            this->set(value);
        }

        void operator =(const char* value)
        {
            this->set(value, strlen(value));
        }

        void operator =(const std::string& value)
        {
            this->set(value.data(), value.length());
        }

        bool operator <(const CharN<N>& value) const
        {
            return this->compare(value) < 0;
        }

        bool operator <=(const CharN<N>& value) const
        {
            return this->compare(value) <= 0;
        }

        bool operator >(const CharN<N>& value) const
        {
            return this->compare(value) > 0;
        }

        bool operator >=(const CharN<N>& value) const
        {
            return this->compare(value) >= 0;
        }

        char& operator [](std::size_t index)
        {
            return this->raw[this->width - 1 - index];
        }

        const char& operator [](std::size_t index) const
        {
            return this->raw[this->width - 1 - index];
        }
    };

    template<std::size_t N>
    std::ostream& operator <<(std::ostream& os, const CharN<N>& value)
    {
        os << (std::string)value;
        return os;
    }


    struct Date
    {
        int32_t raw;

        Date();
        Date(const unsigned year, const unsigned month, const unsigned day);
        unsigned getYear() const;
        unsigned getMonth() const;
        unsigned getDay() const;
        std::string toString() const;
        bool operator ==(const Date& value) const;
        bool operator !=(const Date& value) const;
        bool operator <(const Date& value) const;
        bool operator <=(const Date& value) const;
        bool operator >(const Date& value) const;
        bool operator >=(const Date& value) const;
    };

    std::ostream& operator <<(std::ostream& os, const Date& value);


    struct DateTime
    {
        int64_t raw;

        DateTime();
        DateTime(const unsigned year, const unsigned month, const unsigned day,
                 const unsigned hour, const unsigned minute, const unsigned second, const unsigned millisecond);
        unsigned getYear() const;
        unsigned getMonth() const;
        unsigned getDay() const;
        unsigned getHour() const;
        unsigned getMinute() const;
        unsigned getSecond() const;
        unsigned getMillisecond() const;
        std::string toString() const;
        bool operator ==(const DateTime& value) const;
        bool operator !=(const DateTime& value) const;
        bool operator <(const DateTime& value) const;
        bool operator <=(const DateTime& value) const;
        bool operator >(const DateTime& value) const;
        bool operator >=(const DateTime& value) const;
    };

    std::ostream& operator <<(std::ostream& os, const DateTime& value);


    struct Time
    {
        uint32_t raw;

        Time();
        Time(const unsigned hour, const unsigned minute, const unsigned second, const unsigned millisecond);
        unsigned getHour() const;
        unsigned getMinute() const;
        unsigned getSecond() const;
        unsigned getMillisecond() const;
        std::string toString() const;
        bool operator ==(const Time& value) const;
        bool operator !=(const Time& value) const;
        bool operator <(const Time& value) const;
        bool operator <=(const Time& value) const;
        bool operator >(const Time& value) const;
        bool operator >=(const Time& value) const;
    };

    std::ostream& operator <<(std::ostream& os, const Time& value);


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
            bool isMapped() const;

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
            void seek(const std::size_t pos);

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
            void lock(const bool exclusive);
            void unlock();

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
                DATETIME  = 0x0000200,
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

        const std::string& getStatus() const;
        void setStatus(const std::string& value);

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
        std::string m_status;
        MemoryMappedFile m_statusFile;

        ProcData();
        ProcData(const ProcData&);
        ~ProcData();
        ProcData& operator=(const ProcData&);
        void init();
    };
}

#endif
