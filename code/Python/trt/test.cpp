#include <cuda_runtime_api.h>

#include <fstream>
#include <iostream>
#include <vector>

#include "NvInfer.h"
#include "NvInferRuntime.h"

#define CHECK(status)                                    \
  do {                                                   \
    auto ret = (status);                                 \
    if (ret != 0) {                                      \
      std::cout << "Cuda failure: " << ret << std::endl; \
      abort();                                           \
    }                                                    \
  } while (0)

class Logger : public nvinfer1::ILogger {
  void log(Severity severity, const char* msg) override {
    // suppress info-level messages
    if (severity != Severity::kINFO) std::cout << msg << std::endl;
  }
} gLogger;

int main(int argc, char** argv) {
  if (argc != 2) {
    std::cout << "Pass filename of trt engine as an argument" << std::endl;
    return 0;
  }
  nvinfer1::IRuntime* runtime = nvinfer1::createInferRuntime(gLogger);

  std::string engineName = argv[1];
  std::ifstream engineFile(engineName, std::ios::binary);
  if (!engineFile) {
    std::cout << "Error opening engine file: " << engineName << std::endl;
    return -1;
  }

  engineFile.seekg(0, engineFile.end);
  long int fsize = engineFile.tellg();
  engineFile.seekg(0, engineFile.beg);

  std::vector<char> engineData(fsize);
  engineFile.read(engineData.data(), fsize);
  if (!engineFile) {
    std::cout << "Error loading engine file: " << engineName << std::endl;
    return 0;
  }

  nvinfer1::ICudaEngine* engine =
      runtime->deserializeCudaEngine(engineData.data(), fsize, nullptr);
  nvinfer1::IExecutionContext* context = engine->createExecutionContext();

  // input and output buffer pointers that we pass to the engine - the engine
  // requires exactly ICudaEngine::getNbBindings(), of these, but in this case
  // we know that there is exactly one input and one output.
  if (!(engine->getNbBindings() == 2)) {
    std::cout << "Wrong number of outputs: engine has "
              << engine->getNbBindings() << "outputs" << std::endl;
  }
  void* buffers[2];
  int inputIndex = engine->getBindingIndex("input_1:0");
  int outputIndex = engine->getBindingIndex("Identity:0");

  // allocate GPU buffers
  //constexpr int batchSize = 1; # the batch size is already contained in inputDims.d[0]
  nvinfer1::Dims3 inputDims = static_cast<nvinfer1::Dims3&&>(
                      engine->getBindingDimensions(inputIndex)),
                  outputDims = static_cast<nvinfer1::Dims3&&>(
                      engine->getBindingDimensions(outputIndex));
  std::cout << inputDims.d[0] << "," << inputDims.d[1] << "," << inputDims.d[2]
            << "\n"
            << outputDims.d[0] << "," << outputDims.d[1] << ","
            << outputDims.d[2] << std::endl;
  size_t inputSize =
      inputDims.d[0] * inputDims.d[1] * inputDims.d[2] * sizeof(float);
  size_t outputSize =
      outputDims.d[0] * outputDims.d[1] * outputDims.d[2] * sizeof(float);

  std::cout << "inputSize: " << inputSize << "\t" << "outputSize: " << outputSize << std::endl;
  CHECK(cudaMalloc(&buffers[inputIndex], inputSize));
  CHECK(cudaMalloc(&buffers[outputIndex], outputSize));

  // set the input buffer to some desired value.
  // We use all ones instead of all zeroes to avoid only seeing the biases in case of a single layer network.
  float input_array[inputDims.d[1] * inputDims.d[2]];
  std::fill_n(input_array, inputDims.d[1] * inputDims.d[2], 1.0);
  CHECK(cudaMemcpy(buffers[inputIndex], input_array, inputSize, cudaMemcpyDefault));

  for (int i = 0; i < 10; i++)
    context->executeV2(buffers);  // execute inference
  
  float* dst = new float[6];
  CHECK(cudaMemcpy(dst, buffers[outputIndex], outputSize, cudaMemcpyDefault));
  
  for (int i = 0; i < 6; ++i)
    std::cout << dst[i] << std::endl;


  // release the context and buffers
  context->destroy();                    // Release the execution context
  CHECK(cudaFree(buffers[inputIndex]));  // Release the requested GPU mem buffer
  CHECK(cudaFree(buffers[outputIndex]));

  return 0;
}
