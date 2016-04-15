%module(directors="1",dirprot="1",allprotected="1") Global

%{
#include <StdAfx.h>
%}

%ignore TypeInfo;

%import "ForwardDeclarations.i"

%include "csharp.swg"
%include "typemaps.i"
%apply float &INOUT { float &fSwigRef1 };
%apply float* INOUT { float* fSwigRef1 };
%apply float &INOUT { float &fSwigRef2 };
%apply float* INOUT { float* fSwigRef2 };
%apply float &INOUT { float &fSwigRef3 };
%apply float* INOUT { float* fSwigRef3 };
%apply float &INOUT { float &fSwigRef4 };
%apply float* INOUT { float* fSwigRef4 };
%apply float &INOUT { float &fSwigRef5 };
%apply float* INOUT { float* fSwigRef5 };
%apply bool &INOUT { bool &bSwigRef1 };
%apply bool* INOUT { bool* bSwigRef1 };
%apply bool &INOUT { bool &bSwigRef2 };
%apply bool* INOUT { bool* bSwigRef2 };

%typemap(ctype) char * szOutPath "char *" 
%typemap(imtype) char * szOutPath  "System.Text.StringBuilder" 
%typemap(cstype) char * szOutPath "System.Text.StringBuilder" 
%apply char * szOutPath { char *OUTPUT }