#include <iostream>
#include <assert.h>
#include "SMGFileConverter/FbxLoader.h"
using namespace std;

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        assert(false && L"인자 수가 이상합니다.");
        return -1;
    }
	const string mode = argv[1];
	const string scope = argv[2];
	const string objectFolderPath = argv[3];
    if (mode == "fbxToXml")
	{
        FbxLoader fbxLoader;
        HRESULT rv = fbxLoader.LoadFbxFiles(scope, objectFolderPath);
        if (FAILED(rv))
        {
            assert(false && L"Fbx File load에 실패했습니다.");
            return -1;
        }
        
        //rv = fbxLoader.WriteXmlFiles(objectFolderPath);
        if (FAILED(rv))
        {
			assert(false && L"Fbx File->loadXml Write에 실패했습니다.");
			return -1;
        }

    }
    else if (mode == "xmlToBinary")
	{
    }
    else
    {
		assert(false && L"mode 인자가 이상합니다.");
		return -1;
    }

}