// AddressKey.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "AddressKey.h"

namespace AddressKeyDll
{
	// FUNCTION constructor
	// Establishes a database connection
	AddressKey::AddressKey() {
		db = new DbConnectionDll::DbConnection("localhost", "root", "makaton", "watts_app");
	}
	
	// FUNCTION destructor
	// Deletes the database connection
	AddressKey::~AddressKey() {
		delete db;
	}

	// FUNCTION: generateAddressKey
	// Creates a short unique key for a given address
	string AddressKey::GenerateAddressKey(vector<string>& address) {

		// check validity of pcod 
		string pcod = UtilitiesDll::Utilities::ToUpper(address.back());
		int validPcode = validPostcode(pcod);
		if (0 == validPcode)
			pcod = "";
		else
			address.pop_back(); // remove pcod from address array

		// concatenate all non-postcode address lines into a single string
		// of metaphone tokens delimited by semicolons
		string addressStr = createAddressStr(address);
		string addressMetaStr = MetaphoneDll::Metaphone::PHPMetaLine(addressStr);

		int bestMatchIdx = 0;
		int lowestPosn = -1;

		// if we have a full valid postcode
		if (1 == validPcode) {

			// generate a sort format postcode for keying into the akey table 
			pcod = sortFormat(pcod);

			// retrive the database records for the given postcode
			stringstream query;
			query << "SELECT * FROM akeys WHERE postcode = \"" << pcod << "\"";
			sql::Statement* stmt = db->createStatement();
			sql::ResultSet* matches = db->executeQuery(stmt, query.str());

			// find the database record where the metaphone of the most significant
			// thorofare/locality element matches the metaphone of the least significant
			// thorofare/locality element of the given address
			while (matches->next()) { // for each matching akey

				// can we match the standardised last thorofare/locality string?
				string stdLastStr = matches->getString("stdLastStr");
				int p = (int)addressMetaStr.find(stdLastStr);

				// if not, is there a standardised last welsh thorofare/locality string
				// and can we match it?
				if (string::npos == p) {
					string welsh = matches->getString("stdLastWelshStr");
					if (welsh.length() > 0)
						p = (int)addressMetaStr.find(welsh);
				}

				// if not, is there a recognised abbreviation for the last thorofare/locality string
				// and can we match it?
				if (string::npos == p) {
					string abbrev = buildAbbreviatedLastStr(matches->getString("stdLastStr"));
					if (abbrev.length() > 0)
						p = (int)addressMetaStr.find(abbrev);
				}

				// record this match if better than before
				if ((string::npos != p) && ((lowestPosn < 0) || (p < lowestPosn))) {
					lowestPosn = p;
					bestMatchIdx = matches->getInt("idx");
				}
			}
			delete matches;
			delete stmt;
		}

		// if the matching was successful find the idx of the best matching database
		// record and generate the remainder of the given address after the match
		if (bestMatchIdx > 0)
			addressMetaStr = (lowestPosn > 0 ? addressMetaStr.substr(0, lowestPosn - 1) : "");

		// build the returned key from the above elements
		stringstream retStr;
		if (bestMatchIdx != 1)
			retStr << bestMatchIdx;
		retStr << condensedPcode(pcod) << numbers(addressStr);
		retStr << UtilitiesDll::Utilities::ReplaceAll(addressMetaStr, ";", "");
		return retStr.str();
	}

	// FUNCTION: sortFmtPostcode
	// Format a postcode into a standardised form so it can be sorted
	// Useful for formatting the key into the akey table
	string AddressKey::sortFormat(string givenPostcode)
	{
		// Tidy & sanity check
		givenPostcode = condensedPcode(givenPostcode);
		if (givenPostcode.length() < 5) // shortest postcode is W11AA
			return givenPostcode;

		// Divide given postcode into outcode & incode
		string outcode = givenPostcode.substr(0, givenPostcode.length() - 3);
		string incode = givenPostcode.substr(givenPostcode.length() - 3);

		// The outcode has an alpha 'area' part followed by a numeric 'district' part
		// Usually the outcode is separated from the incode by a space, but sometimes
		// the separator is an alpha, eg. E1W1AA
		string outcodeElms[] = { "", "", "" };
		int outcodeElmIdx = 0;
		char lastCharType = 'A'; // 'A' = alpha, '9' = digit
		for (size_t i = 0; i < outcode.length(); i++) {
			char c = outcode.at(i);
			char thisCharType = isdigit(c) ? '9' : 'A';
			if (thisCharType != lastCharType) {
				outcodeElmIdx++;
				lastCharType = thisCharType;
			}
			outcodeElms[outcodeElmIdx] += c;
		}
		outcodeElms[2] += " "; // make sure there is at least a 1 space separator

		stringstream sortFormatPc;
		sortFormatPc << left << setw(2) << setfill(' ') << outcodeElms[0].substr(0, 2);
		sortFormatPc << right << setw(2) << setfill('0') << outcodeElms[1].substr(0, 2);
		sortFormatPc << setw(1) << setfill(' ') << outcodeElms[2].substr(0, 1);
		sortFormatPc << incode;

		return sortFormatPc.str();
	}

	// FUNCTION: condensedPcode
	// Return the shortest possible version of a postcode 
	string AddressKey::condensedPcode(string givenPostcode)
	{
		givenPostcode = UtilitiesDll::Utilities::Trim(givenPostcode);
		string retOutStr, retNumStr, retInStr;
		for (unsigned i = 0; i < givenPostcode.length(); i++) {
			char c = givenPostcode.at(i);
			if (isalpha(c)) {
				if (retNumStr.length() < 1)
					retOutStr += c;
				else
					retInStr += c;
				continue;
			}
			if (isdigit(c))
				retNumStr += c;
		}

		while ((retNumStr.length() > 2) && ('0' == retNumStr.at(0)))
			retNumStr = retNumStr.substr(1);

		if (3 == retInStr.length()) {
			switch (retInStr.at(0)) {
			case 'O':
				retInStr = '0' + retInStr.substr(1);
				break;
			case 'I':
			case 'L':
				retInStr = '1' + retInStr.substr(1);
				break;
			case 'S':
				retInStr = '5' + retInStr.substr(1);
				break;
			case 'B':
				retInStr = '8' + retInStr.substr(1);
				break;
			default:
				break;
			}
		}

		return retOutStr + retNumStr + retInStr;
	}

	// Function validPostcode
	// Checks a given string to see if it could be a postcode
	int AddressKey::validPostcode(string& given) {
		// Set up a map of valid patterns
		static map<string, int> mValidPatterns = {
			{ "A", -1 },
			{ "A9", -1 },
			{ "A99", -1 },
			{ "A999", -1 },
			{ "A999A", -1 },
			{ "A999AA", 1 },
			{ "A99A", -1 },
			{ "A99A9", -1 },
			{ "A99A9A", -1 },
			{ "A99A9AA", 1 },
			{ "A99AA", 1 },
			{ "A9A", -1 },
			{ "A9A9", -1 },
			{ "A9A9A", -1 },
			{ "A9A9AA", 1 },
			{ "AA", -1 },
			{ "AA9", -1 },
			{ "AA99", -1 },
			{ "AA999", -1 },
			{ "AA999A", -1 },
			{ "AA999AA", 1 },
			{ "AA99A", -1 },
			{ "AA99A9", -1 },
			{ "AA99A9A", -1 },
			{ "AA99A9AA", 1 },
			{ "AA99AA", 1 },
			{ "AA9A", -1 },
			{ "AA9A9", -1 },
			{ "AA9A9A", -1 },
			{ "AA9A9AA", 1 }
		};
		
		// Tidy & sanity check
		given = condensedPcode(given);
		if ((given.length() < 5) || (given.length() > 8)) // shortest postcode is W11AA, longest is WA11W1AA
			return false;

		// Codify the given postcode
		string coded = codifyStr(given);

		// Make a decision based on the resulting coded string
		// If the encoded string isn't in the valid map, return 0
		// If the encoded string is in the valid map, return 1 if full and -1 if partial
		map<string, int>::iterator iValid = mValidPatterns.find(coded);
		if (iValid == mValidPatterns.end())
			return 0;
		return iValid->second;
	}

	// Function codifyStr
	// Codifies a string so that 'A' represents an alpha segment, '9' represents a numberic segment
	// and 'O' represents all other cases
	string AddressKey::codifyStr(string given) {
		string retStr = "";
		for (size_t i = 0; i < given.length(); i++) {
			char thisChar = given.at(i);
			char thisCode = (isalpha(thisChar) ? 'A' : (isdigit(thisChar) ? '9' : 'O'));
			retStr += thisCode;
		}
		return retStr;
	}

	// FUNCTION: createAddressStr
	// Concatenate all elements of an address array into a single string
	string AddressKey::createAddressStr(vector<string>& vAddress) {
		stringstream addLine;
		for(vector<string>::iterator iAddress = vAddress.begin(); iAddress != vAddress.end(); iAddress++){
			if (addLine.str().length() > 0)
				addLine << " ";
			addLine << *iAddress;
		}
		return addLine.str();
	}


	// FUNCTION: numbers
	// Return a string containing just the digits for the input
	string AddressKey::numbers(string given) {
		string numStr = "";
		string separator = "";
		string separatorSymbol = ".";
		for (size_t i = 0; i < given.length(); i++) {
			char c = given.at(i);
			if (isdigit(c)) {
				numStr += separator + c;
				separator = "";
			}
			else
				separator = separatorSymbol;
		}

		// Remove leading & trailing dots
		if (numStr.length() > 0) {
			if (numStr.substr(0, 1) == separatorSymbol)
				numStr = numStr.substr(1);
			if (numStr.substr(numStr.length() - 1) == separatorSymbol)
				numStr = numStr.substr( 0, numStr.length() - 1);
		}

		return numStr;
	}

	// FUNCTION buildAbbreviatedLastStr
	// Takes a semicolon delimted, metaphone representation of thorofare/locality
	// and replaces the last token with its abbreviated equivalent if one exists.
	// Otherwise, a null string is returned
	string AddressKey::buildAbbreviatedLastStr(string original) {
		map<string, string> abbreviations = {
			/*
			{"RD", "RD"}, // Road
			{"LN", "LN"}, // Lane
			*/
			{"KLS", "KL"}, // Close
			{"PLS", "PL"}, // Place
			{"STRT", "ST"} //Street
		};

		// find the start of the last token
		size_t lastSemiColon = original.find_last_of(";");
		size_t startOfLastWord = ((lastSemiColon == string::npos) ? 0 : lastSemiColon + 1);

		// separate the last token from the others
		string previousWords = original.substr(0, startOfLastWord);
		string lastWord = original.substr(startOfLastWord);

		// substitute the abbreviation for the last token if there is one
		map<string, string>::iterator iFoundAbbreviation = abbreviations.find(lastWord);
		if (iFoundAbbreviation != abbreviations.end())
			return previousWords.append(iFoundAbbreviation->second);

		return "";
	}

}

