#include "stdafx.h"
#include "Exception.h"

DxException::DxException()
	: _hr(S_OK)
	, _errorCode(ErrCode::Success)
	, _functionName()
	, _fileName()
	, _lineNumber(-1)
{
}

DxException::DxException(HRESULT hr, ErrCode errorCode, const std::wstring& functionName, const std::wstring& fileName, const int lineNumber)
	: _hr(hr)
	, _errorCode(errorCode)
	, _functionName(functionName)
	, _fileName(fileName)
	, _lineNumber(lineNumber)
{
}

std::wstring DxException::to_wstring() const noexcept
{
	_com_error err(_hr);
	return _functionName + L" failed in " + _fileName + L": line " + std::to_wstring(_lineNumber) + L"\n"
		+ err.ErrorMessage() + L" " + ErrCodeWString(_errorCode) + L"\n";
}
std::string DxException::to_string() const noexcept
{
	USES_CONVERSION;
	std::string rv = W2A(to_wstring().c_str());
	return rv;
}
