#include "kinetica/Proc.hpp"

#include <iostream>

int main(int argc, char *argv[])
{
    try
    {
        kinetica::ProcData* procData = kinetica::ProcData::get();
        const kinetica::ProcData::InputDataSet& inputData = procData->getInputData();
        kinetica::ProcData::OutputDataSet& outputData = procData->getOutputData();

        // Loop through input and output tables (assume the same number)
        for (size_t i = 0; i < inputData.getTableCount(); ++i)
        {
            const kinetica::ProcData::InputTable& inputTable = inputData[i];
            kinetica::ProcData::OutputTable& outputTable = outputData[i];
            outputTable.setSize(inputTable.getSize());

            // Loop through columns in the input and output tables (assume the same number and types)
            for (size_t j = 0; j < inputTable.getColumnCount(); ++j)
            {
                const kinetica::ProcData::InputColumn& inputColumn = inputTable[j];
                kinetica::ProcData::OutputColumn& outputColumn = outputTable[j];

                // For each record, copy the data from the input column to the output column
                for (size_t k = 0; k < inputTable.getSize(); ++k)
                {
                    if (inputColumn.isNull(k))
                    {
                        outputColumn.appendNull();
                        continue;
                    }

                    switch (inputColumn.getType())
                    {
                        case kinetica::ProcData::Column::BYTES: outputColumn.appendVarBytes(inputColumn.getVarBytes(k)); break;
                        case kinetica::ProcData::Column::CHAR1: outputColumn.appendValue(inputColumn.getValue<kinetica::CharN<1> >(k)); break;
                        case kinetica::ProcData::Column::CHAR2: outputColumn.appendValue(inputColumn.getValue<kinetica::CharN<2> >(k)); break;
                        case kinetica::ProcData::Column::CHAR4: outputColumn.appendValue(inputColumn.getValue<kinetica::CharN<4> >(k)); break;
                        case kinetica::ProcData::Column::CHAR8: outputColumn.appendValue(inputColumn.getValue<kinetica::CharN<8> >(k)); break;
                        case kinetica::ProcData::Column::CHAR16: outputColumn.appendValue(inputColumn.getValue<kinetica::CharN<16> >(k)); break;
                        case kinetica::ProcData::Column::CHAR32: outputColumn.appendValue(inputColumn.getValue<kinetica::CharN<32> >(k)); break;
                        case kinetica::ProcData::Column::CHAR64: outputColumn.appendValue(inputColumn.getValue<kinetica::CharN<64> >(k)); break;
                        case kinetica::ProcData::Column::CHAR128: outputColumn.appendValue(inputColumn.getValue<kinetica::CharN<128> >(k)); break;
                        case kinetica::ProcData::Column::CHAR256: outputColumn.appendValue(inputColumn.getValue<kinetica::CharN<256> >(k)); break;
                        case kinetica::ProcData::Column::DATE: outputColumn.appendValue(inputColumn.getValue<kinetica::Date>(k)); break;
                        case kinetica::ProcData::Column::DATETIME: outputColumn.appendValue(inputColumn.getValue<kinetica::DateTime>(k)); break;
                        case kinetica::ProcData::Column::DECIMAL: outputColumn.appendValue(inputColumn.getValue<int64_t>(k)); break;
                        case kinetica::ProcData::Column::DOUBLE: outputColumn.appendValue(inputColumn.getValue<double>(k)); break;
                        case kinetica::ProcData::Column::FLOAT: outputColumn.appendValue(inputColumn.getValue<float>(k)); break;
                        case kinetica::ProcData::Column::INT: outputColumn.appendValue(inputColumn.getValue<int32_t>(k)); break;
                        case kinetica::ProcData::Column::INT8: outputColumn.appendValue(inputColumn.getValue<int8_t>(k)); break;
                        case kinetica::ProcData::Column::INT16: outputColumn.appendValue(inputColumn.getValue<int16_t>(k)); break;
                        case kinetica::ProcData::Column::IPV4: outputColumn.appendValue(inputColumn.getValue<uint32_t>(k)); break;
                        case kinetica::ProcData::Column::LONG: outputColumn.appendValue(inputColumn.getValue<int64_t>(k)); break;
                        case kinetica::ProcData::Column::STRING: outputColumn.appendVarString(inputColumn.getVarString(k)); break;
                        case kinetica::ProcData::Column::TIME: outputColumn.appendValue(inputColumn.getValue<kinetica::Time>(k)); break;
                        case kinetica::ProcData::Column::TIMESTAMP: outputColumn.appendValue(inputColumn.getValue<int64_t>(k)); break;
                    }
                }
            }
        }

        // Copy any parameters from the input parameter map into the output results map (not necessary for table copying, just for illustrative purposes)
        std::map<std::string, std::string> params = procData->getParams();
        procData->getResults().insert(params.begin(), params.end());
        std::map<std::string, std::vector<uint8_t> > binParams = procData->getBinParams();
        procData->getBinResults().insert(binParams.begin(), binParams.end());

        // Inform Kinetica that the proc has finished successfully
        procData->complete();
    }
    catch (const std::exception& ex)
    {
        std::cout << "ERROR: " << ex.what() << std::endl;
        return 1;
    }

    return 0;
}
