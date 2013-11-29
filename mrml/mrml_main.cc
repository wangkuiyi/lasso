

//
#include "base/common.h"
#include "mrml/mrml.h"

extern bool MRML_Initialize(int argc, char** argv);
extern bool MRML_AmIMapWorker();
extern void MRML_MapWork();
extern void MRML_ReduceWork();
extern void MRML_Finalize();

//-----------------------------------------------------------------------------
// The pre-defined main function
//-----------------------------------------------------------------------------

int main(int argc, char** argv) {
  if (!MRML_Initialize(argc, argv)) {
    return -1;
  }

  LOG(INFO) << "I am a "
            << (MRML_AmIMapWorker() ?
                string("map worker") : string("reduce worker"));

  if (MRML_AmIMapWorker()) {
    MRML_MapWork();
  } else {
    MRML_ReduceWork();
  }

  MRML_Finalize();
  return 0;
}
