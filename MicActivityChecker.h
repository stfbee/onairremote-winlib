#ifndef INDLL_H
#define INDLL_H

#ifdef EXPORTING_DLL
      extern __declspec(dllexport) BOOL is_microphone_recording() ;
#else
      extern __declspec(dllimport) BOOL is_microphone_recording();
#endif

#endif
