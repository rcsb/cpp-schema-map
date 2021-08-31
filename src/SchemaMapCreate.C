/*
 *  SchemaMapCreate.C
 */


#include <string.h>
#include <iostream>
#include "CifString.h"
#include "GenCont.h"
#include "CifFile.h"
#include "DicFile.h"
#include "CifFileUtil.h"

#include "SchemaMap.h"

using std::cout;
using std::cerr;
using std::endl;


//  This definintion influences the construction of auxillary tables tableinfo
//  and columinfo used by NDB data management tools, and it controls the
//  insertion of an implicit key in all tables.
//
/*  JDW added default values for V5_next/rc
"int-range", "point_symmetry", "id_list",  "3x4_matrices", "non_negative_int", "positive_int", "emd_id", "pdb_id", "point_group", "point_group_helical", "boolean", "author", "orcid_id",  "symmetry_operation", ""
     "char",           "char",    "char",        "char",              "int",          "int",   "char",   "char",        "char",                "char",    "char",   "char",     "char",                "char", ""
      "25",              "20",      "80",         "100",               "10",           "10",     "10",     "15",          "20",                  "20",       "5",     "80",       "20",                  "80", ""
       "0",               "0",       "0",           "0",                "0",            "0",      "0",      "0",           "0",                   "0",       "0",      "0",        "0",                   "0", ""
       "3",                 3",       "5",           "5",                "1",            "1",      "3",      "3",           "3",                   "3",       "3",      "5",        "3",                   "3", ""
*/

static const char* cifTypes[] =
{
    "code", "ucode", "line", "uline", "text", "int", "float", "name",
    "idname", "any", "yyyy-mm-dd", "uchar3", "uchar1", "symop", "atcode",
    "yyyy-mm-dd:hh:mm", "fax", "phone", "email", "code30", "float-range",
    "operation_expression", "yyyy-mm-dd:hh:mm-flex", "ec-type","ucode-alphanum-csv",
    "int-range", "point_symmetry", "id_list",  "3x4_matrices", "non_negative_int", "positive_int",
    "emd_id", "pdb_id", "point_group", "point_group_helical", "boolean", "author", "orcid_id",  "symmetry_operation",
    "exp_data_doi", "asym_id", "id_list_spc", ""
};

static const char* sqlTypes[] =
{
    "char", "char", "char", "char", "text", "int", "float", "char",
    "char", "text", "datetime", "char", "char", "char", "char",
    "datetime", "char", "char", "char", "char", "char",
    "char" , "datetime", "char", "char",
    "char",        "char",    "char",        "char",              "int",          "int",
    "char",   "char",        "char",                "char",    "char",   "char",     "char",                "char",
    "char", "char", "char", ""
};

static const char* defFieldWidths[] =
{
    "10", "10", "80", "80", "200", "10", "10", "80",
    "80", "255", "15", "4", "2", "10", "6",
    "20", "25", "25", "80", "30", "30",
    "30", "20", "10", "25",
    "20", "80",         "100",               "10",           "10",     "10",
    "15", "20", "20", "5", "80", "20", "80", "80",
    "80", "80", "200", ""
};

static const char* defFieldPrecisions[] =
{
    "0", "0", "0", "0", "0", "0", "6", "0",
    "0", "0", "0", "0", "0", "0", "0",
    "0", "0", "0", "0", "0", "0",
    "0", "0", "0", "0",
    "0",  "0", "0", "0", "0", "0",
    "0", "0", "0", "0", "0",  "0", "0", "0",
    "0", "0", "0", ""
};

static const char* formatTypes[] =
{
    "3", "3", "5", "5", "5", "1", "2", "3",
    "3", "5", "7", "3", "3", "3", "3",
    "7", "3", "3", "3", "3", "3",
    "3", "3", "3", "3",
    "3", "5", "5", "1", "1", "3",
    "3", "3", "3", "3", "5", "3", "3", "5",
    "3", "3", "5", ""
};

static string CurrAliasName;
static vector<string> QueryColAlias;

string SchemaMap::_DATABLOCK_ID("datablock_id");
string SchemaMap::_DATABASE_2("database_2");
string SchemaMap::_ENTRY_ID ("entry_id");
string SchemaMap::_STRUCTURE_ID_COLUMN("Structure_ID");

static int get_a_line(string& line, const string& s, int& iPos);
static unsigned int count_leading_ws(const string& line);
static unsigned int nws_strlen(const string& line);
static unsigned int format_cif_text(string& sout, const string& string);

static bool mungCifName(const string& cifName, string& dbName);


SchemaMap::SchemaMap(const eFileMode fileMode, const string& dictSdbFileName,
  const bool verbose)
{
    _verbose = verbose;

    _fobjS = new CifFile(false, Char::eCASE_SENSITIVE, 255);

    _fobjS->AddBlock("rcsb_schema");
    _fobjS->AddBlock("rcsb_schema_map");


    _dict = GetDictFile(NULL, string(), dictSdbFileName, _verbose);

    GetDictTables();

    //  Define/create the tables in the output mapping file...
    CreateMappingTables();
}


void SchemaMap::Create(const string& op, const string& inFile,
  const string& structId)
{
    vector<string> tableList;
    vector<vector<string> > allColNames;
    GetTablesAndColumns(tableList, allColNames, op, inFile);

    cout << "Done creating dictionary tables" << endl;

    //   Iterate over the categories and items in the dictionary file.
    for (unsigned int it = 0; it < tableList.size(); ++it)
    {
        // tableList[it] - contains the current category

        if (_verbose)
            cout << endl << "ADDING TABLE " << tableList[it] << endl;

        vector<string> categoryKeys;
        GetCategoryKeys(categoryKeys, tableList[it]);

        string dbTableNameAbbrev;
        AbbrevTableName(dbTableNameAbbrev, tableList[it]);

        UpdateTableSchema(tableList[it]);

        UpdateTableDescr(tableList[it]);

        // Process a special column for non-info tables
        if ((dbTableNameAbbrev != "tableinfo") &&
          (dbTableNameAbbrev != "columninfo"))
        {
            UpdateAttribSchema(tableList[it], _STRUCTURE_ID_COLUMN, 0,
              categoryKeys);

            UpdateAttribDescr(tableList[it], _STRUCTURE_ID_COLUMN);

            UpdateAttribView(tableList[it], _STRUCTURE_ID_COLUMN, 0);

            UpdateSchemaMap(tableList[it], _STRUCTURE_ID_COLUMN, structId);
        }

        // colNames contains all the attribute names that belong to the
        // category tableList[it]
        vector<string> colNames;

        if (inFile.empty())
        {
            // List of items from Dictionary.
            GetColumnsFromDict(colNames, tableList[it]);
        }
        else
        {
            // List of items from the input file.
            colNames = allColNames[it];
        }

        for (unsigned int ic = 0; ic < colNames.size(); ++ic)
        {
            if (_verbose)
                cout << "   Adding column " << colNames[ic] << endl;

            AbbrevColName(tableList[it], colNames[ic]);

            unsigned int itype = GetTypeCodeIndex(tableList[it], colNames[ic]);

            UpdateAttribSchema(tableList[it], colNames[ic], itype,
              categoryKeys);

            UpdateAttribDescr(tableList[it], colNames[ic]);

            UpdateAttribView(tableList[it], colNames[ic], itype);

            MapAlias(tableList[it], colNames[ic], op);
        } // For each column in table
    } // For each table

    tableList.clear();

    UpdateMapConditions();

    WriteMappingTables();
}

void SchemaMap::Write(const string& fileName)
{
    _fobjS->Write(fileName);
}

unsigned int format_cif_text(string& sout, const string& inString)
{

    sout.clear();

    unsigned int nLines = 0;

    if (inString.empty())
        return(nLines);

    int iPos = 0;
    unsigned int indent = 512;
    unsigned int i;
    string line;

    /*
     *  Get the minimum indentation for non blank lines.
     */
    while (get_a_line(line, inString, iPos))
    {
        if (nws_strlen(line))
        {
            i = count_leading_ws(line);
            if (i < indent)
                indent = i;
        }
        nLines++;
    }

    iPos = 0;
    i = 0;
    while (get_a_line(line, inString, iPos))
    {
        if (nws_strlen(line))
        {
            if ((i==0) && (indent != 0))
            {
	        sout += line.substr(indent-1);
                i++;
            }
            else
	        sout += line.substr(indent);
        }
        else
            sout += "\n";
    }

    return(nLines);
}


unsigned int nws_strlen(const string& line)
{
    unsigned int len = 0;

    if (line.empty())
        return(len);

    for (unsigned int i = 0; i < line.size(); ++i)
    {
        if (!Char::IsWhiteSpace(line[i]))
            len++;
    }

    return(len);
}


unsigned int count_leading_ws(const string& line)
{
    unsigned int i = 0;

    if (line.empty())
        return(i);

    while (Char::IsWhiteSpace(line[i]))
    {
        i++;
    }
    return(i);
}


int get_a_line(string& line, const string& s, int& iPos)
  /*
   * Get the next new line terminated string in s[iPos] and return
   * this in line[]. Update the nPos. Return line length.
   */
{
    line.clear();

    int i = iPos;
    int j = 0;

    while ((s[i] != '\n') && (s[i] != '\0'))
    {
        line.push_back(s[i]);
        i++;
        j++;
    }

    if (s[i] == '\n')
    {
        line.push_back('\n');
        j++;
        i++;
    }

    iPos = i;

    return(j);
}

void SchemaMap::UpdateTableDescr(const string& category)
{

    vector<string> queryTarget;
    vector<string> queryCol;

    queryTarget.push_back(category);
    queryCol.push_back("id");

    unsigned int foundIndex = _categoryTab->FindFirst(queryTarget, queryCol);

    string categoryDescription;

    if (foundIndex == _categoryTab->GetNumRows())
    {
        cout << "Warning - category "<< category <<
          ".  Category description is not in dictionary." << endl;
    }
    else
    {
        const string& ctmp = (*_categoryTab)(foundIndex, "description");
        unsigned int nLines = format_cif_text(categoryDescription, ctmp);

        if (nLines == 1)
            String::rcsb_clean_string(categoryDescription);

        if (_verbose)
            cout << "Description for "<< category << " is:" << endl <<
	      categoryDescription << endl;
    }

    vector<string> newRowT5;

    newRowT5.push_back(category);
    newRowT5.push_back(categoryDescription);

    _tableDescr->AddRow(newRowT5);

}


void SchemaMap::UpdateTableSchema(const string& category)
{

    vector<string> queryTarget;
    vector<string> queryCol;

    queryTarget.push_back(category);
    queryCol.push_back("category_id");

    vector<unsigned int> queryResult;
    _categoryGroupTab->Search(queryResult, queryTarget, queryCol);

    string categoryGroup;
    if (queryResult.empty() || queryResult.size() < 2)
    {
        cout << "Warning - category " << category <<
          ".  No group defined for this category." << endl;
    }
    else
    {
        categoryGroup =
          (*_categoryGroupTab)(queryResult[queryResult.size() - 1], "id");

        if (_verbose)
            cout << "First group for " << category << " is:" << endl <<
              categoryGroup << endl;
    }

    vector<string> newRowT6;

    newRowT6.push_back(category);
    newRowT6.push_back("0");
    newRowT6.push_back(categoryGroup);
    newRowT6.push_back("1");
    newRowT6.push_back("1");

    _tableSchema->AddRow(newRowT6);

}


void SchemaMap::AbbrevTableName(string& dbTableNameAbbrev,
  const string& tableName)
{

    // ----------------------------------------------------------
    //  SQL table name truncation rule and reserved word handling
    //            Store truncation in translation table
    bool iTabAbbrev = mungCifName(tableName, dbTableNameAbbrev);

    if (iTabAbbrev)
    {
        vector<string> newRowT8;

        newRowT8.push_back(tableName);
        newRowT8.push_back(dbTableNameAbbrev);

        _tableAbbrev->AddRow(newRowT8);
    }

}

void SchemaMap::AbbrevColName(const string& tableName, const string& colName)
{
    // --------------------------------------------------------------
    //  SQL column name truncation and reserved char/word handling
    //  This ONLY deals with cases that have been observed in mmCIF
    //  (e.g. [,],-)
    string dbColNameAbbrev;
    bool iColAbbrev = mungCifName(colName, dbColNameAbbrev);
    if (iColAbbrev)
    {
        vector<string> newRowT9;

        newRowT9.push_back(tableName);
        newRowT9.push_back(colName);
        newRowT9.push_back(dbColNameAbbrev);

        _attribAbbrev->AddRow(newRowT9);
    }
}


void SchemaMap::UpdateAttribSchema(const string& tableName,
  const string& colName, const unsigned int itype,
  const vector<string>& categoryKeys)
{

    string itemName;
    CifString::MakeCifItem(itemName, tableName, colName);

    string key;

    if (colName == _STRUCTURE_ID_COLUMN)
    {
        key = "1";
    }
    else
    {
        // Assume non-key item
        key = "0";
        if (GenCont::IsInVector(itemName, categoryKeys))
        {
            key = "1";
        }
    }

    vector<string> newRowT1;

    newRowT1.push_back(tableName);
    newRowT1.push_back(colName);
    newRowT1.push_back(sqlTypes[itype]);
    newRowT1.push_back(key);
    newRowT1.push_back(key);
    newRowT1.push_back(defFieldWidths[itype]);
    newRowT1.push_back(defFieldPrecisions[itype]);

    _attribSchema->AddRow(newRowT1);

}

void SchemaMap::UpdateAttribView(const string& tableName,
  const string& colName, const unsigned int itype)
{
    vector<string> newRowT3;

    newRowT3.push_back(tableName);
    newRowT3.push_back(colName);
    newRowT3.push_back(formatTypes[itype]);
    newRowT3.push_back("1");
    newRowT3.push_back("1");

    _attribView->AddRow(newRowT3);
}

void SchemaMap::UpdateAttribDescr(const string& tableName,
  const string& colName)
{

    string itemDescription;
    string firstItemExample;

    if (colName == _STRUCTURE_ID_COLUMN)
    {
        itemDescription = "Structure identifier";
        firstItemExample = "Structure identifier";
    }
    else
    {
        string itemName;
        CifString::MakeCifItem(itemName, tableName, colName);

        GetItemDescription(itemDescription, itemName);

        GetFirstItemExample(firstItemExample, itemName);
    }

    vector<string> newRowT2;

    newRowT2.push_back(tableName);
    newRowT2.push_back(colName);
    newRowT2.push_back(itemDescription);
    newRowT2.push_back(firstItemExample);

    _attribDescr->AddRow(newRowT2);

}

void SchemaMap::UpdateSchemaMap(const string& tableName, const string& colName,
  const string& structId)
{

    vector<string> newRowT4;

    newRowT4.push_back(tableName);
    newRowT4.push_back(colName);

    if (structId == _ENTRY_ID)
    {
        newRowT4.push_back("_struct.entry_id");
        newRowT4.push_back("C1");
        newRowT4.push_back(CifString::UnknownValue);
    }
    else if (structId == _DATABLOCK_ID)
    {
        // use datablock() as Structure_ID
        newRowT4.push_back(CifString::UnknownValue);
        newRowT4.push_back(CifString::UnknownValue);
        newRowT4.push_back("datablockid()");
    }
    else if (structId == _DATABASE_2)
    {
        // use a particular database code as Structure_ID
        newRowT4.push_back("_database_2.database_code");
        newRowT4.push_back("CDB");
        newRowT4.push_back(CifString::UnknownValue);
    }

    _schemaMap->AddRow(newRowT4);

}


void SchemaMap::MapAlias(const string& tableName, const string& colName,
  const string& op)
{

    string itemName;
    CifString::MakeCifItem(itemName, tableName, colName);

    string aliasName = itemName;

    if ((op == "alias") || (op == "unalias"))
    {
        vector<string> queryTargetAlias;

        queryTargetAlias.push_back(itemName);
        queryTargetAlias.push_back("cif_rcsb.dic");
        queryTargetAlias.push_back("1.1");

        unsigned int foundIndex =
          _itemAliasesTab->FindFirst(queryTargetAlias, QueryColAlias);
        if (foundIndex != _itemAliasesTab->GetNumRows())
        {
            aliasName = (*_itemAliasesTab)(foundIndex, CurrAliasName);
        }

        if (_verbose)
            cout << "   Alias name is " << aliasName << endl;
    }

    vector<string> newRowT4;

    newRowT4.push_back(tableName);
    newRowT4.push_back(colName);
    newRowT4.push_back(aliasName);
    newRowT4.push_back(CifString::UnknownValue);
    newRowT4.push_back(CifString::UnknownValue);

    _schemaMap->AddRow(newRowT4);

}

void SchemaMap::GetItemDescription(string& itemDescription, const string& itemName)
{
    vector<string> queryTarget;
    vector<string> queryCol;

    queryTarget.push_back(itemName);
    queryCol.push_back("name");

    unsigned int foundIndex =
      _itemDescriptionTab->FindFirst(queryTarget, queryCol);
    if (foundIndex == _itemDescriptionTab->GetNumRows())
    {
        //  No result  ...
        cout << "   Warning - item "<< itemName <<
          ".  Item description is NOT in dictionary." << endl;
    }
    else
    {
        itemDescription =
          (*_itemDescriptionTab)(foundIndex, "description");

        string ctmp;
        unsigned int nLines = format_cif_text(ctmp, itemDescription);
        if (nLines == 1)
            String::rcsb_clean_string(ctmp);
        itemDescription = ctmp;
        if (_verbose)
            cout << "   Item description for " << itemName << " is: " <<
              endl << itemDescription << endl;
    }
}

void SchemaMap::GetFirstItemExample(string& firstItemExample,
  const string& itemName)
{

    //   Get leading item example  ...
    vector<string> queryTarget;
    vector<string> queryCol;

    queryTarget.push_back(itemName);
    queryCol.push_back("name");

    unsigned int foundIndex =
      _itemExamplesTab->FindFirst(queryTarget, queryCol);
    if (foundIndex == _itemExamplesTab->GetNumRows())
    {
        //  No result  ...
        if (_verbose)
            cout << "   Warning - item "<< itemName <<
              ".  No item examples in dictionary." << endl;

        firstItemExample = "Missing example";
    }
    else
    {
        firstItemExample = (*_itemExamplesTab)(foundIndex, "case");
        if (_verbose)
            cout << "   Item example for " << itemName << " is: " <<
              endl << firstItemExample << endl;
    }

}


void SchemaMap::GetTopParentName(string& topParentName, const string& itemName)
{

    // ----------------------------------------------------------------
    //  Get parent item name and ultimate parent item name if
    // appropriate

    string parentName;

    vector<string> queryTarget;
    vector<string> queryCol;

    queryTarget.push_back(itemName);
    queryCol.push_back("child_name");

    vector<unsigned int> queryResult;
    _itemLinkedTab->Search(queryResult, queryTarget, queryCol);
    if (queryResult.empty())
    { //  No result  ...

    }
    else
    {
        if (queryResult.size() > 1)
        {
            cout << "   Warning - item \""<< itemName <<
              "\".  Item has multiple parents." << endl;
        }

        parentName = (*_itemLinkedTab)(queryResult[0], "parent_name");

        if (_verbose)
            cout << "   Parent for item " << itemName << " is " <<
              parentName << endl;
    }
    queryResult.clear();

    // VLAD - what is meant by the next line of code
    if (!parentName.empty())
    {
        topParentName = parentName;
        string tmpParentName = parentName;
        while (!tmpParentName.empty())
        {
            queryTarget.clear();
            queryTarget.push_back(topParentName);

            queryCol.clear();
            queryCol.push_back("child_name");

            _itemLinkedTab->Search(queryResult, queryTarget, queryCol);

            if (queryResult.empty())
            { //  No result  ...
                tmpParentName.clear();
            }
            else
            {
                if (queryResult.size() > 1)
                {
                    cout << "   Warning - item \"" << tmpParentName <<
                      "\".  Item has multiple parents." << endl;
                }

                tmpParentName =
                  (*_itemLinkedTab)(queryResult[0], "parent_name");

                if (_verbose)
                    cout << "   Parent for item " << topParentName <<
                      " is " << tmpParentName << endl;
                topParentName = tmpParentName;
            }
            queryResult.clear();
        }
    }

}


void SchemaMap::GetTypeCode(string& typeCode, const string& topParentName,
  const string& itemName)
{

    vector<string> queryTarget;
    vector<string> queryCol;

    queryTarget.push_back(itemName);
    queryCol.push_back("name");

    unsigned int foundIndex = _itemTypeTab->FindFirst(queryTarget, queryCol);
    if (foundIndex == _itemTypeTab->GetNumRows())
    { //  No result  ...
#ifndef VLAD_NEW_BEHAVIOUR
        if (_verbose)
            cout << "   Warning - item " << itemName <<
              ".  Item type is NOT in dictionary." << endl;
#else
        if (inFile.empty())
        {
            cerr << "++ ERROR - " << itemName <<
              ".  Item type is NOT in dictionary." << endl;
        }
        else
        {
            if (_verbose)
                cout << "   Warning - item " << itemName <<
                  ".  Item type is NOT in dictionary." << endl;
        }
#endif
    }
    else
    {
        typeCode = (*_itemTypeTab)(foundIndex, "code");
        if (_verbose)
            cout << "   Item type for " << itemName << " is " <<
              typeCode << endl;
    }

    // --------------------------------------------------------------
    //   Use ultimate parent type if required.

    if (typeCode.empty() && !topParentName.empty())
    {
        queryTarget.clear();
        queryTarget.push_back(topParentName);
        queryCol.clear();
        queryCol.push_back("name");

        unsigned int foundIndex = _itemTypeTab->FindFirst(queryTarget,
          queryCol);
        if (foundIndex == _itemTypeTab->GetNumRows())
        { //  No result  ...
            cout << "   Warning - item "<< topParentName <<
              ".  Parent item type is NOT in dictionary." << endl;
        }
        else
        {
            typeCode = (*_itemTypeTab)(foundIndex, "code");
            if (_verbose)
                cout << "   Item type for " << itemName <<
                  " taken from parent " << topParentName <<
                  " parent type is " << typeCode << endl;
        }
    }

    if (typeCode.empty())
    {
        cerr << "++ ERROR - " << itemName <<
          ".  Item type is NOT in dictionary." << endl;
    }

}

void SchemaMap::GetCategoryKeys(vector<string>& categoryKeys,
  const string& category)
{

    categoryKeys.clear();

    vector<string> queryTarget;
    vector<string> queryCol;

    queryTarget.push_back(category);
    queryCol.push_back("id");

    vector<unsigned int> queryResult;
    _categoryKeyTab->Search(queryResult, queryTarget, queryCol);

    if (queryResult.empty())
    {
        cout << "Warning - category " << category <<
          ".  Category keys are not in dictionary." << endl;
    }
    else
    {
        for (unsigned int i = 0; i < queryResult.size(); ++i)
        {
            const string& cs = (*_categoryKeyTab)(queryResult[i], "name");
            categoryKeys.push_back(cs);
            if (_verbose)
                cout << "Key item in category " << category <<
                  " is:" << endl << cs << endl;
        }
    }

}


void SchemaMap::CreateMappingTables()
{

    _attribSchema = new ISTable("rcsb_attribute_def");
    _attribSchema->AddColumn("table_name");
    _attribSchema->AddColumn("attribute_name");
    _attribSchema->AddColumn("data_type");
    _attribSchema->AddColumn("index_flag");
    _attribSchema->AddColumn("null_flag");
    _attribSchema->AddColumn("width");
    _attribSchema->AddColumn("precision");

    _attribDescr = new ISTable("rcsb_attribute_description");
    _attribDescr->AddColumn("table_name");
    _attribDescr->AddColumn("attribute_name");
    _attribDescr->AddColumn("description");
    _attribDescr->AddColumn("example");

    _attribView = new ISTable("rcsb_attribute_view");
    _attribView->AddColumn("table_name");
    _attribView->AddColumn("attribute_name");
    _attribView->AddColumn("format_type");
    _attribView->AddColumn("www_select");
    _attribView->AddColumn("www_report");

    _schemaMap = new ISTable("rcsb_attribute_map");
    _schemaMap->AddColumn("target_table_name");
    _schemaMap->AddColumn("target_attribute_name");
    _schemaMap->AddColumn("source_item_name");
    _schemaMap->AddColumn("condition_id");
    _schemaMap->AddColumn("function_id");

    _tableDescr = new ISTable("rcsb_table_description");
    _tableDescr->AddColumn("table_name");
    _tableDescr->AddColumn("description");

    _tableSchema = new ISTable("rcsb_table");
    _tableSchema->AddColumn("table_name");
    _tableSchema->AddColumn("table_type");
    _tableSchema->AddColumn("group_name");
    _tableSchema->AddColumn("www_select");
    _tableSchema->AddColumn("www_report");

    _mapConditions = new ISTable("rcsb_map_condition");
    _mapConditions->AddColumn("condition_id");
    _mapConditions->AddColumn("attribute_name");
    _mapConditions->AddColumn("attribute_value");

    _tableAbbrev = new ISTable("rcsb_table_abbrev");
    _tableAbbrev->AddColumn("table_name");
    _tableAbbrev->AddColumn("table_abbrev");

    _attribAbbrev = new ISTable("rcsb_attribute_abbrev");
    _attribAbbrev->AddColumn("table_name");
    _attribAbbrev->AddColumn("attribute_name");
    _attribAbbrev->AddColumn("attribute_abbrev");
}


void SchemaMap::WriteMappingTables()
{
    Block& rsBlock = _fobjS->GetBlock("rcsb_schema");

    rsBlock.WriteTable(_tableAbbrev);
    rsBlock.WriteTable(_attribAbbrev);
    rsBlock.WriteTable(_tableSchema);
    rsBlock.WriteTable(_attribSchema);

    rsBlock.WriteTable(_tableDescr);
    rsBlock.WriteTable(_attribDescr);
    rsBlock.WriteTable(_attribView);

    Block& rsmBlock = _fobjS->GetBlock("rcsb_schema_map");

    rsmBlock.WriteTable(_schemaMap);
    rsmBlock.WriteTable(_mapConditions);
}

#ifdef VLAD_DELETED
void SchemaMappingDestructor()
{

    _fobjS->Close();

}
#endif

void SchemaMap::GetDictTables()
{
    // --------------------------------------------------------------------
    //     Extract the following DDL categories from the data dictionary...

    Block& block = _dict->GetBlock(_dict->GetFirstBlockName());

    _itemTypeTab = block.GetTablePtr("item_type");
    if (_itemTypeTab == NULL)
    {
        throw NotFoundException("Dictionary table item_type is missing.",
          "SchemaMap::GetDictTables");
    }

    _itemLinkedTab = block.GetTablePtr("item_linked");
    if (_itemLinkedTab == NULL)
    {
        throw NotFoundException("Dictionary table item_linked is missing.",
          "SchemaMap::GetDictTables");
    }

    _itemDescriptionTab = block.GetTablePtr("item_description");
    if (_itemDescriptionTab == NULL)
    {
        throw NotFoundException("Dictionary table item_description is "\
          "missing.", "SchemaMap::GetDictTables");
    }

    _itemExamplesTab = block.GetTablePtr("item_examples");
    if (_itemExamplesTab == NULL)
    {
        throw NotFoundException("Dictionary table item_examples is missing.",
          "SchemaMap::GetDictTables");
    }

    _categoryTab = block.GetTablePtr("category");
    if (_categoryTab == NULL)
    {
        throw NotFoundException("Dictionary table category is missing.",
          "SchemaMap::GetDictTables");
    }

    _categoryGroupTab = block.GetTablePtr("category_group");
    if (_categoryGroupTab == NULL)
    {
        throw NotFoundException("Dictionary table category_group is missing.",
          "SchemaMap::GetDictTables");
    }

    _categoryKeyTab = block.GetTablePtr("category_key");
    if (_categoryKeyTab == NULL)
    {
        throw NotFoundException("Dictionary table category_key is missing.",
          "SchemaMap::GetDictTables");
    }

    _itemTab = block.GetTablePtr("item");
    if (_itemTab == NULL)
    {
        throw NotFoundException("Dictionary table item is missing.",
          "SchemaMap::GetDictTables");
    }

    _itemAliasesTab = block.GetTablePtr("item_aliases");

    vector<string> searchCols;
    searchCols.push_back("id");

#ifdef RCSB_TABLEINFO_COLUMNINFO
    vector<unsigned int> matchedRows;

    vector<string> target;
    target.push_back("rcsb_tableinfo");

    _categoryTab->Search(matchedRows, target, searchCols);
    if (matchedRows.empty())
    {
        throw NotFoundException("Auxiliary table \"rcsb_tableinfo\""\
         " is missing. Concatenate \"sql-fragment.dic\" to the dictionary.",
          "SchemaMap::GetDictTables");
    }

    target.clear();
    target.push_back("rcsb_columninfo");

    _categoryTab->Search(matchedRows, target, searchCols);
    if (matchedRows.empty())
    {
        throw NotFoundException("Auxiliary table \"rcsb_columninfo\""\
         " is missing. Concatenate \"sql-fragment.dic\" to the dictionary.",
          "SchemaMap::GetDictTables");
    }
#endif // RCSB_TABLEINFO_COLUMNINFO

}


void SchemaMap::GetTablesAndColumns(vector<string>& tableList,
  vector<vector<string> >& allColNames, const string& op, const string& inFile)
{
    if (!inFile.empty())
    {
        // Get the name of items from the specified file and not from the
        // dictionary
        CifFile* fobjIn = ParseCif(inFile, _verbose);

        const string& parsingDiags = fobjIn->GetParsingDiags();

        if (!parsingDiags.empty())
        {
            cout << "Diags for file " << fobjIn->GetSrcFileName() <<
              "  = " << parsingDiags << endl;
        }

        vector<string> blockNames;
        fobjIn->GetBlockNames(blockNames);
        unsigned int nBlocks = blockNames.size();
        for (unsigned int ib=0; ib < nBlocks; ib++)
        {
            if (blockNames[ib].empty())
                // VLAD - WARNING - ISSUE SOME WARNING HERE
                continue;

            Block& inBlock = fobjIn->GetBlock(blockNames[ib]);

            if (_verbose)
                cout << endl << "PROCESSING BLOCK " << blockNames[ib] << endl;

            vector<string> blockTableList;
            inBlock.GetTableNames(blockTableList);

            tableList.insert(tableList.end(), blockTableList.begin(),
              blockTableList.end());

            unsigned int nTables = blockTableList.size();
            for (unsigned int it=0; it < nTables; it++)
            {
                ISTable* t = inBlock.GetTablePtr(blockTableList[it]);

                allColNames.push_back(t->GetColumnNames());
            }
        }

        delete (fobjIn);
    }
    else
    {
        _categoryTab->GetColumn(tableList, "id");

        cout << "Category table has " << _categoryTab->GetNumRows() <<
          " entries." << endl;
    }

    if ((op != "alias") && (op != "unalias"))
    {
        return;
    }

    if (_itemAliasesTab == NULL)
    {
        throw NotFoundException("Dictionary table item_aliases is missing.",
          "SchemaMap::MapAlias");
    }

    if (op == "alias")
    {
        // alias - search name extract alias name
        CurrAliasName = "alias_name";

        _itemAliasesTab->SetFlags("name", ISTable::DT_STRING |
          ISTable::CASE_INSENSE);

        QueryColAlias.push_back("name");
    }
    else // (op == "unalias")
    {
        // unalias - search alias name extract name
        CurrAliasName = "name";

        _itemAliasesTab->SetFlags("alias_name", ISTable::DT_STRING |
          ISTable::CASE_INSENSE);

        QueryColAlias.push_back("alias_name");
    }

    QueryColAlias.push_back("dictionary");
    QueryColAlias.push_back("version");
}


void SchemaMap::GetColumnsFromDict(vector<string>& colNames,
  const string& tableName)
{

    colNames.clear();

    // List of items from Dictionary.
    vector<string> queryTarget;
    vector<string> queryCol;

    queryTarget.push_back(tableName);
    queryCol.push_back("category_id");

    vector<unsigned int> queryResult;
    _itemTab->Search(queryResult, queryTarget, queryCol);

    if (queryResult.empty())
    {
        //  No result
        cout << "Warning - Table "<< tableName <<
          ".  No items in dictionary." << endl;
    }
    else
    {
        for (unsigned int i = 0; i < queryResult.size(); ++i)
        {
            string itemName = (*_itemTab)(queryResult[i], "name");
            string keyword;
            CifString::GetItemFromCifItem(keyword, itemName);
            colNames.push_back(keyword);
        }
    }

}


unsigned int SchemaMap::GetTypeCodeIndex(const string& tableName,
  const string& colName)
{

    string itemName;
    CifString::MakeCifItem(itemName, tableName, colName);

    string topParentName;
    GetTopParentName(topParentName, itemName);

    string typeCode;
    GetTypeCode(typeCode, topParentName, itemName);

    // ---------------------------------------------------------------
    //   Map CIF data types to SQL types

    unsigned int maxCifType = 0;
    while (strlen(cifTypes[maxCifType]))
    {
        maxCifType++;
    }

    unsigned int itype = 0;
    while (strlen(cifTypes[itype]))
    {
        if (!strcmp(cifTypes[itype], typeCode.c_str()))
        {
            break;
        }
        itype++;
    }

    if (itype >= maxCifType || typeCode.size() < 3)
    {
        cerr << "++ERROR \"" << typeCode <<  "\" type unknown " << endl;

        //throw NotFoundException("Unknown type code: \"" + typeCode + "\"",
        //  "SchemaMap::GetTypeCodeIndex");
        //
        cerr << "++WARN \"" << typeCode <<  "\" treated as TEXT type  -- revise code for better handling --" << endl;
        itype = 4;
    }

    return(itype);
}


void SchemaMap::UpdateMapConditions()
{
    vector<string> newRowT7;
    newRowT7.push_back("C1");
    newRowT7.push_back("entry_id");
    newRowT7.push_back("datablockid()");
    _mapConditions->AddRow(newRowT7);

    newRowT7.clear();
    newRowT7.push_back("CDB");
    newRowT7.push_back("database_id");
    newRowT7.push_back("PDB");
    _mapConditions->AddRow(newRowT7);
}


#define NDBCOMPAT 1

bool mungCifName(const string& cifName, string& dbNameOut)
{
    dbNameOut.clear();

    int i,j,iLen, mLen, mDbLen;
    char dbName[256];
    mDbLen = 60;
    mLen=255;
    memset(dbName,'\0',256);
    iLen = cifName.size();
    bool iReturn = false;
    if (iLen > mDbLen)
    {
        iReturn = true;
        j=0;
        for (i=0; i < iLen; i++)
        {
            if (j >= mLen)
                continue;

            if (cifName[i] == '[' || cifName[i] == ']' )
                continue;

            if (cifName[i] == '-' || cifName[i] == '_' )
            {
	        if (i < iLen - 1)
                {
	            dbName[j] = Char::ToUpper(cifName[i+1]); i++;
	        }
                else
                {
	            continue;
	        }
            }
            else
            {
	        dbName[j] = cifName[i];
            }
            j++;
        }

        if ((int) strlen(dbName) > mDbLen)
        {
            //JDW -  heuristic substitutions to cleanup munging in PDB Ex dictionary  - -
            string tS;
            tS = dbName;
            unsigned int il;
            il=tS.find("Constraint");
            if (il < tS.size())
                tS.replace(tS.find("Constraint"),10,"Cnstr");
            il=tS.find("Distance");
            if (il < tS.size())
                tS.replace(tS.find("Distance"),8,"Dst");
            il=tS.find("Total");
            if (il < tS.size())
	        tS.replace(tS.find("Total"),5,"Tl");
            il=tS.find("Correction");
            if (il < tS.size())
	        tS.replace(tS.find("Correction"),10,"Cor");
            il=tS.find("Location");
            if (il < tS.size())
	        tS.replace(tS.find("Location"),8,"Loc");
            il=tS.find("Count");
            if (il < tS.size())
                tS.replace(tS.find("Count"),5,"Ct");
            il=tS.find("Violation");
            if (il < tS.size())
	        tS.replace(tS.find("Violation"),9,"Viol");
            il=tS.find("Cutoff");
            if (il < tS.size())
	        tS.replace(tS.find("Cutoff"),6,"Ctff");
            il=tS.find("Residue");
            if (il < tS.size())
	        tS.replace(tS.find("Residue"),7,"Res");
            il=tS.find("Torsion");
            if (il < tS.size())
                tS.replace(tS.find("Tor"),8,"Res");
            il=tS.find("Angle");
            if (il < tS.size())
                tS.replace(tS.find("Angle"),5,"Res");
            il=tS.find("Stereochemistry");
            if (il < tS.size())
	        tS.replace(tS.find("Stereochemistry"),15,"Ster");
            il=tS.find("Stereochem");
            if (il < tS.size())
	        tS.replace(tS.find("Stereochem"),10,"Ster");
            il=tS.find("Average");
            if (il < tS.size())
                tS.replace(tS.find("Average"),7,"Avg");
            il=tS.find("Averaging");
            if (il < tS.size())
	        tS.replace(tS.find("Averaging"),9,"Avg");
            il=tS.find("Maximum");
            if (il < tS.size())
	        tS.replace(tS.find("Maximum"),7,"Max");
            il=tS.find("Minimum");
            if (il < tS.size())
	        tS.replace(tS.find("Minimum"),7,"Min");
            il=tS.find("maximum");
            if (il < tS.size())
	        tS.replace(tS.find("maximum"),7,"max");
            il=tS.find("minimum");
            if (il < tS.size())
                tS.replace(tS.find("minimum"),7,"min");
            il=tS.find("Radius");
            if (il < tS.size())
                tS.replace(tS.find("Radius"),6,"Rad");
            il=tS.find("Gyration");
            if (il < tS.size())
                tS.replace(tS.find("Gyration"),8,"Gyr");
            il=tS.find("VanDerWaals");
            if (il < tS.size())
                tS.replace(tS.find("VanDerWaals"),11,"VDW");
            il=tS.find("Potential");
            if (il < tS.size())
                tS.replace(tS.find("Potential"),9,"Pot");
            il=tS.find("CrossSectional");
            if (il < tS.size())
	        tS.replace(tS.find("CrossSectional"),14,"CrSct");
            il=tS.find("Correlation");
            if (il < tS.size())
	        tS.replace(tS.find("Correlation"),11,"Cor");
            il=tS.find("Parameter");
            if (il < tS.size())
	        tS.replace(tS.find("Parameter"),9,"Parm");
            il=tS.find("Recombination");
            if (il < tS.size())
	        tS.replace(tS.find("Recombination"),13,"Recmb");
            il=tS.find("Timepoint");
            if (il < tS.size())
	        tS.replace(tS.find("Timepoint"),9,"Tmpt");
            il=tS.find("Express");
            if (il < tS.size())
	        tS.replace(tS.find("Express"),7,"Expr");
            il=tS.find("Bonded");
            if (il < tS.size())
                tS.replace(tS.find("Bonded"),6,"Bnd");
            il=tS.find("Units");
            if (il < tS.size())
                tS.replace(tS.find("Units"),5,"Un");
            il=tS.find("Size");
            if (il < tS.size())
                tS.replace(tS.find("Size"),4,"Sz");
            memset(dbName,'\0',256);
            for (i=0; i < (int) tS.size(); i++)
            {
	        dbName[i] = tS.c_str()[i];
            }
            if ((int)tS.size() > mDbLen )
                cout << "WARNING - " << tS << " exceeds character length of " << mDbLen << endl;
        }
    }
    else
    {
        j=0;
        for (i=0; i < iLen; i++)
        {
            if ( cifName[i] == ']')
            {
	        iReturn=true;
	        continue;
            }
            if (cifName[i] == '[' || cifName[i] == '-' || cifName[i] == '%' ||
	      cifName[i] == '/')
            {
	        dbName[j] = '_';
	        j++;
	        iReturn = true;
	        continue;
            }
            dbName[j] = cifName[i];
            j++;
        }
    }

    // Update result...
    iLen = strlen(dbName);
    if (iLen > mDbLen) iLen = mDbLen;
    for (i=0; i < iLen; i++) {
      dbNameOut.push_back(dbName[i]);
    }

    // Some special table handling...

    if ((dbNameOut == "database") || (dbNameOut == "cell") ||
      (dbNameOut == "order") || (dbNameOut == "partition") ||
      (dbNameOut == "group"))
    {
        dbNameOut += "1";
        iReturn = true;
    }

    // --------------------------------------------------------------------------------
    //   Special tables required for NDB compatability ...

    if (NDBCOMPAT)
    {
        if ((dbNameOut == "rcsb_tableinfo") || (dbNameOut == "pdbx_tableinfo"))
        {
          dbNameOut = "tableinfo";
          iReturn= true;
        }
        if ((dbNameOut == "rcsb_columninfo") ||
          (dbNameOut == "pdbx_columninfo"))
        {
          dbNameOut = "columninfo";
          iReturn = true;
        }
    }

    return (iReturn);
}

