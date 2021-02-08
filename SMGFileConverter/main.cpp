#include <iostream>
#include "SMGFileConverter/FbxLoader.h"
#include "SMGEngine/D3DUtil.h"
using namespace std;

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        check(false, "인자 수가 이상합니다.");
        return -1;
    }
	const string mode = argv[1];
	const string scope = argv[2];
	const string objectFolderPath = argv[3];
    if (mode == "fbxToXml")
	{
        try
		{
			FbxLoader fbxLoader;
			fbxLoader.ConvertFbxFiles(scope, objectFolderPath);
        }
        catch (DxException& e)
        {
			MessageBox(nullptr, e.to_wstring().c_str(), L"초기화 실패 !", MB_RETRYCANCEL);
			return 1;
        }
    }
    else if (mode == "xmlToBinary")
	{
    }
    else
    {
		check(false, "mode 인자가 이상합니다.");
		return -1;
    }

}