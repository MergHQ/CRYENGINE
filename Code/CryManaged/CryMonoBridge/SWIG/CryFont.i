%include "CryEngine.swig"

%import "CryCommon.i"

%{
#include <CryFont/IFont.h>
%}

%ignore CreateCryFontInterface;
%include "../../../../CryEngine/CryCommon/CryFont/IFont.h"