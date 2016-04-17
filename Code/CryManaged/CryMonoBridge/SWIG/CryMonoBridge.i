%include "CryEngine.swig"

%import "CryCommon.i"

%{
#include <CryMono/IMonoRuntime.h>
%}
%feature("director") IMonoListener;
%include "../../../../CryEngine/CryCommon/CryMono/IMonoRuntime.h"