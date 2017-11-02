#pragma once
// AddressKey.h

#ifdef ADDRESSKEY_EXPORTS
#define ADDRESSKEY_API __declspec(dllexport) 
#else
#define ADDRESSKEY_API __declspec(dllimport) 
#endif

#pragma comment(lib, "C:/Users/Rob/Documents/Visual Studio 2015/Projects/Utilities/Debug/Utilities.lib")
#pragma comment(lib, "C:/Users/Rob/Documents/Visual Studio 2015/Projects/Metaphone/Debug/Metaphone.lib")
#pragma comment(lib, "C:/Users/Rob/Documents/Visual Studio 2015/Projects/DbConnection/Debug/DbConnection.lib")
#pragma comment(lib, "C:/Users/Rob/Documents/Visual Studio 2015/Projects/DbConnection/DbConnection/mysqlcppconn.lib")

#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <iomanip>
#include "C:/Users/Rob/Documents/Visual Studio 2015/Projects/Utilities/Utilities/Utilities.h"
#include "C:/Users/Rob/Documents/Visual Studio 2015/Projects/Metaphone/Metaphone/Metaphone.h"
#include "C:/Users/Rob/Documents/Visual Studio 2015/Projects/DbConnection/DbConnection/DbConnection.h"

using namespace std;

namespace AddressKeyDll
{
	class AddressKey
	{
	public:
		ADDRESSKEY_API AddressKey();
		ADDRESSKEY_API ~AddressKey();
		ADDRESSKEY_API string GenerateAddressKey(vector<string>& address);
		ADDRESSKEY_API string AddressKey::sortFormat(string givenPostcode);

	private:
		string condensedPcode(string givenPostcode);
		string createAddressStr(vector<string>& vAddress);
		string numbers(string given);
		string buildAbbreviatedLastStr(string original);
		int validPostcode(string& given);
		string codifyStr(string given);

		DbConnectionDll::DbConnection* db;
	};
}

