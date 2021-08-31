/*$$FILE$$*/
/*$$VERSION$$*/
/*$$DATE$$*/
/*$$LICENSE$$*/

 
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits>

#include "CifParserBase.h"
#include "CifFileUtil.h"
#include "SchemaMap.h"


using std::numeric_limits;
using std::cout;
using std::endl;


static void _set_attribute_value_where(ISTable* t,  const string& category,
  const string& attribute, const string& attributeChange,
  const string& valueChange);


const long _MAXFILESIZE = std::numeric_limits<std::streamsize>::max();


SchemaMap::SchemaMap(const string& schemaFile,
  const string& schemaFileOdb, bool verbose)
{
  Clear();

  // Schema file is required.
  if (schemaFile.empty() && schemaFileOdb.empty())
      return;

  _schemaFile = schemaFile;

  _schemaFileOdb = schemaFileOdb;

  //
  //----------------------------------------------------------------------
  //              Read schema and map data file ... 
  //

  if (_schemaFileOdb.empty())
  {
      _fobjS = ParseCif(_schemaFile, _verbose, Char::eCASE_SENSITIVE,
        _MAX_LINE_LENGTH);

        const string& parsingDiags = _fobjS->GetParsingDiags();

        if (!parsingDiags.empty())
        {
            cout << "Diags for file " << _fobjS->GetSrcFileName() <<
              "  = " << parsingDiags << endl;
        }
  }
  else if (!_schemaFile.empty())
  {
      _fobjS = new CifFile(CREATE_MODE, _schemaFileOdb, _verbose,
        Char::eCASE_SENSITIVE, _MAX_LINE_LENGTH);

      _fobjS->SetSrcFileName(_schemaFile);

      CifParser cifParser(_fobjS, _verbose);

      cifParser.Parse(_schemaFile, _fobjS->_parsingDiags);

      const string& parsingDiags = _fobjS->GetParsingDiags();

      if (!parsingDiags.empty())
      {
          cout << parsingDiags;
          throw NotFoundException("Possibly file not found \"" + _schemaFile +
            "\"", "SchemaMap::SchemaMap");
      }
  }
  else
  {
      _fobjS = new CifFile(READ_MODE, _schemaFileOdb, _verbose,
        Char::eCASE_SENSITIVE, _MAX_LINE_LENGTH);
  }

  _AssignAttribIndices();
}


SchemaMap::~SchemaMap()
{

    if (_fobjS != NULL)
    {
        // Note that all tables that are in _fobjS will also be deleted here.
        delete(_fobjS);
    }

#ifdef VLAD_RESOLVE_MEMORY_LEAK_SEG_FAULT
    // In suite, this causes segmentation fault. If removed it causes
    // memory leak in db-loader
    delete(_schemaMap);
#endif

}


void SchemaMap::Clear()
{

    _schemaFile.clear();
    _schemaFileOdb.clear();

    _fobjS = NULL;

    _tableSchema = NULL;
    _attribSchema = NULL;
    _schemaMap = NULL;
    _mapConditions = NULL;
    _tableAbbrev = NULL;
    _attribAbbrev = NULL;
    _reviseSchemaMode  = false;

    _verbose = false;

}


void SchemaMap::GetAllTablesNames(vector<string>& tablesNames)
{
    _tableSchema->GetColumn(tablesNames, "table_name");
}


void SchemaMap::GetDataTablesNames(vector<string>& tablesNames)
{
    tablesNames.clear();

    vector<string> allTablesNames;
    GetAllTablesNames(allTablesNames);

    for (unsigned int i = 0; i < allTablesNames.size(); ++i)
    {
        if (allTablesNames[i].empty() ||
          (allTablesNames[i] == "rcsb_tableinfo") ||
          (allTablesNames[i] == "rcsb_columninfo"))
            continue;

        tablesNames.push_back(allTablesNames[i]);
    }
}


void SchemaMap::GetGroupNames(vector<string>& groupNames)
{

    _tableSchema->GetColumn(groupNames, "group_name");

}


void SchemaMap::GetAttributeNames(vector<string>& attributeNames, 
  const string& tableName)
{

    attributeNames.clear();

    vector<unsigned int> ir;

    _attribSchema->Search(ir, tableName, "table_name");
    if (ir.empty())
        return;

    _attribSchema->GetColumn(attributeNames, "attribute_name", ir);

}


const vector<AttrInfo>& SchemaMap::GetAttributesInfo(const string& tableName)
{
    if (tableName == _tableName)
    {
        return (_attrInfo);
    }

    _tableName = tableName;

    _attrInfo.clear();

    vector<unsigned int> ir;

    _attribSchema->Search(ir, tableName, "table_name");

    for (unsigned int i = 0; i < ir.size(); ++i)
    {
        AttrInfo tmpAttrInfo;
 
        tmpAttrInfo.attribName = (*_attribSchema)(ir[i], "attribute_name");
        tmpAttrInfo.dataType = (*_attribSchema)(ir[i], "data_type");
        tmpAttrInfo.iTypeCode = _ConvertDataType(tmpAttrInfo.dataType);
        tmpAttrInfo.iIndex = String::StringToBoolean((*_attribSchema)(ir[i],
          "index_flag"));
        const string& nullFlag = (*_attribSchema)(ir[i], "null_flag");
        tmpAttrInfo.iNull = atoi(nullFlag.c_str());
        const string& width = (*_attribSchema)(ir[i], "width");
        tmpAttrInfo.iWidth = atoi(width.c_str());
        const string& precision = (*_attribSchema)(ir[i], "precision");
        tmpAttrInfo.iPrecision = atoi(precision.c_str());
        tmpAttrInfo.populated = (*_attribSchema)(ir[i], "populated");

        _attrInfo.push_back(tmpAttrInfo);
    }

    return (_attrInfo);
}


void SchemaMap::GetMappedAttributesInfo(
  vector<vector<string> >& mappedAttrInfo, const string& tableName)
{

    mappedAttrInfo.clear();
 
    vector<unsigned int> irMap;

    // Find indices in attribute map of mapped attributes of the table
    _schemaMap->Search(irMap, tableName, "target_table_name");
    if (irMap.empty())
        return;

    vector<string> tmpVector;
    mappedAttrInfo.push_back(tmpVector);
    mappedAttrInfo.push_back(tmpVector);
    mappedAttrInfo.push_back(tmpVector);
    mappedAttrInfo.push_back(tmpVector);

    // Get mapped attribute names of this table
    _schemaMap->GetColumn(mappedAttrInfo[0], "target_attribute_name", irMap);

    // Get source CIF item names of the mapped attribute names of this table
    _schemaMap->GetColumn(mappedAttrInfo[1], "source_item_name", irMap);

    // Get condition id and function id of the mapped attribute names of
    //  this table
    _schemaMap->GetColumn(mappedAttrInfo[2], "condition_id", irMap);
    _schemaMap->GetColumn(mappedAttrInfo[3], "function_id", irMap);

}


void SchemaMap::GetMappedConditions(
  vector<vector<string> >& mappedConditions, const string& tableName)
{

    mappedConditions.clear();
 
    vector<unsigned int> ir;

    _mapConditions->Search(ir, tableName, "condition_id");

    if (ir.empty())
        return;

    vector<string> tmpVector;
    mappedConditions.push_back(tmpVector);
    mappedConditions.push_back(tmpVector);

    _mapConditions->GetColumn(mappedConditions[0], "attribute_name", ir);
    _mapConditions->GetColumn(mappedConditions[1], "attribute_value", ir);

}


void SchemaMap::ReviseSchemaMap(const string& mapFile)
{

    if (!_reviseSchemaMode || mapFile.empty())
        return;

    if (_fobjS)
    {
        cout << "Writing revised schema map file " << mapFile << endl;
        Write(mapFile);
    }

}


void SchemaMap::SetReviseSchemaMode(bool mode)
{
    _reviseSchemaMode = mode;
}


bool SchemaMap::GetReviseSchemaMode()
{
    return(_reviseSchemaMode);
}


void SchemaMap::_AssignAttribIndices()
{

  Block& rsBlock = _fobjS->GetBlock("rcsb_schema");
  Block& rsmBlock = _fobjS->GetBlock("rcsb_schema_map");

  _tableSchema     = rsBlock.GetTablePtr("rcsb_table");
  _attribSchema    = rsBlock.GetTablePtr("rcsb_attribute_def");
  _schemaMap       = rsmBlock.GetTablePtr("rcsb_attribute_map");
  _mapConditions   = rsmBlock.GetTablePtr("rcsb_map_condition");

  if (rsBlock.IsTablePresent("rcsb_table_abbrev"))
  {
      _tableAbbrev = rsBlock.GetTablePtr("rcsb_table_abbrev");
      if ((!_tableAbbrev->IsColumnPresent("table_name")) ||
        (!_tableAbbrev->IsColumnPresent("table_abbrev")))
      {
#ifdef VLAD_LOG_SEPARATION
        if (_verbose) _log << "Table abbreviations incomplete" << endl;
#endif
        return;
      }
  }

  if (rsBlock.IsTablePresent("rcsb_attribute_abbrev"))
  {
    _attribAbbrev = rsBlock.GetTablePtr("rcsb_attribute_abbrev");

    if ((!_attribAbbrev->IsColumnPresent("table_name")) ||
      (!_attribAbbrev->IsColumnPresent("attribute_abbrev")) ||
      (!_attribAbbrev->IsColumnPresent("attribute_abbrev")))
    {
#ifdef VLAD_LOG_SEPARATION
      if (_verbose) _log << "Attribute abbreviations incomplete" << endl;
#endif
      return;
    }
  }

  if ((!_tableSchema->IsColumnPresent("table_name")) ||
    (!_tableSchema->IsColumnPresent("group_name")))
  {
#ifdef VLAD_LOG_SEPARATION
    if (_verbose) _log << "Table schema table incomplete" << endl;
#endif
    return;
  }

  if (!_attribSchema->IsColumnPresent("populated"))
  {
    _attribSchema->AddColumn("populated");
  }
  // VLAD else .. = -1

  if ((!_attribSchema->IsColumnPresent("table_name")) ||
    (!_attribSchema->IsColumnPresent("attribute_name")) ||
    (!_attribSchema->IsColumnPresent("data_type")) ||
    (!_attribSchema->IsColumnPresent("index_flag")) ||
    (!_attribSchema->IsColumnPresent("null_flag")) ||
    (!_attribSchema->IsColumnPresent("width")) ||
    (!_attribSchema->IsColumnPresent("precision")))
  {
#ifdef VLAD_LOG_SEPARATION
    if (_verbose) _log << "Attribute schema table incomplete" << endl;
#endif
    return;
  }

  if ((!_schemaMap->IsColumnPresent("target_table_name")) ||
    (!_schemaMap->IsColumnPresent("target_attribute_name")) || 
    (!_schemaMap->IsColumnPresent("source_item_name")) ||
    (!_schemaMap->IsColumnPresent("condition_id")) ||
    (!_schemaMap->IsColumnPresent("function_id")))
  {
#ifdef VLAD_LOG_SEPARATION
    if (_verbose) _log << "Schema map table incomplete" << endl;
#endif
    return;
  }

  if ((!_mapConditions->IsColumnPresent("condition_id")) ||
    (!_mapConditions->IsColumnPresent("attribute_name")) ||
    (!_mapConditions->IsColumnPresent("attribute_value")))
  {
#ifdef VLAD_LOG_SEPARATION
    if (_verbose) _log << "Map condition table incomplete" << endl;
#endif
    return;
  }

  //
  // Check that all schema attributes have a CIF mapping....
  //

  ISTable* t = new ISTable("rcsb_attribute_map1");

  t->AddColumn("target_table_name");
  t->AddColumn("target_attribute_name");
  t->AddColumn("source_item_name");
  t->AddColumn("condition_id");
  t->AddColumn("function_id");

  vector<string> qcol;
  qcol.push_back("table_name");
  qcol.push_back("attribute_name");

  // Verify if category and attribute specified in schema map are specified
  // in attribute schema. If yes, use them. If not discard them.
  for (unsigned int i = 0; i < _schemaMap->GetNumRows(); i++)
  {
    const vector<string>& row = _schemaMap->GetRow(i);

    if (row.empty())
    {
      continue;
    }

    vector<string> qval;    
    qval.push_back((*_schemaMap)(i, "target_table_name"));
    qval.push_back((*_schemaMap)(i, "target_attribute_name"));

    vector<unsigned int> ir;
    _attribSchema->Search(ir, qval, qcol);
    if (!ir.empty())
    {
      t->AddRow(row);
      continue;
    }
    else
    {
      //      cerr << "In " << _INPUT_FILE << ": " << "No schema correspondence for " << "_"<< qval[0].c_str() 
      //	   << "."  <<qval[1].c_str() << endl;
#ifdef VLAD_LOG_SEPARATION
      if (_verbose) _log << "No schema correspondence for " 
			 << ((*_schemaMap)(i, "source_item_name")).c_str() 
			 << endl;
#endif
    }
  }

  _schemaMap = t;

}


void SchemaMap::GetTableNameAbbrev(string& abbrevTableName,
  const string& tableName)
{
    abbrevTableName = tableName;

    if (_tableAbbrev == NULL)
    {
        return;
    }

    vector<string> qcol;
    qcol.push_back("table_name");

    vector<string> qval;
    qval.push_back(tableName);

    unsigned int searchIndex = _tableAbbrev->FindFirst(qval, qcol);

    if (searchIndex != _tableAbbrev->GetNumRows())
    {
        abbrevTableName = (*_tableAbbrev)(searchIndex, "table_abbrev");
    }
}

void SchemaMap::GetAttributeNameAbbrev(string& abbrevAttrName,
  const string& tableName, const string& attribName)
{
    abbrevAttrName = attribName;

    if (_attribAbbrev == NULL)
    {
        return;
    }

    vector<string> qcol;
    qcol.push_back("table_name");
    qcol.push_back("attribute_name");

    vector<string> qval;
    qval.push_back(tableName);
    qval.push_back(attribName);

    unsigned int searchIndex = _attribAbbrev->FindFirst(qval, qcol);
    if (searchIndex != _attribAbbrev->GetNumRows())
    {
        abbrevAttrName = (*_attribAbbrev)(searchIndex, "attribute_abbrev");
    }
}


void SchemaMap::getSchemaMapDetails(const string& tableName,
  const string& attribName, string& width, string& populated)
{
    width.clear();
    populated.clear();

    // Get attibute schema details... 
    vector<string> qcol;
    qcol.push_back("table_name");
    qcol.push_back("attribute_name");

    vector<string> qval;    
    qval.push_back(tableName);
    qval.push_back(attribName);

    unsigned int ir = _attribSchema->FindFirst(qval, qcol);

    width = (*_attribSchema)(ir, "width");
    populated = (*_attribSchema)(ir, "populated");
}


void SchemaMap::updateSchemaMapDetails(SchemaMap& scm)
{

    unsigned int numPopulated = 0;
    unsigned int numPopulatedR = 0;
    unsigned int numPopulatedU = 0;

    unsigned int nRows = _attribSchema->GetNumRows();

    for (unsigned int i = 0; i < nRows; ++i)
    {
        const string& attribName = (*_attribSchema)(i, "attribute_name");
        const string& tableName = (*_attribSchema)(i, "table_name");

        const string& width = (*_attribSchema)(i, "width");
        const string& populated = (*_attribSchema)(i, "populated");

        int iWidth = atoi(width.c_str());

        string widthU, populatedU;

        scm.getSchemaMapDetails(tableName, attribName, widthU, populatedU);

        int iWidthU = atoi(widthU.c_str());

        if (populated == "Y")
            numPopulated++;

        if (populatedU == "Y")
            numPopulatedR++;

        if (populated == "Y" || populatedU == "Y")
        {
            _set_attribute_value_where(_attribSchema, tableName, attribName,
              "populated", "Y");
            numPopulatedU++;
        }
        else
        {
            _set_attribute_value_where(_attribSchema, tableName, attribName,
              "populated", "N");
        }

        if (iWidthU > iWidth)
        {
            _set_attribute_value_where(_attribSchema, tableName, attribName,
              "width", widthU);
        }
    }

    cout << "Items populated in the reference map = " << numPopulated << endl;
    cout << "Items populated in the revised   map = " << numPopulatedR << endl;
    cout << "Items populated in the updated   map = " << numPopulatedU << endl;

}


void SchemaMap::UpdateAttributeDef(const string& tableName,
  const string& columnName, int iType, int iWidth, int newWidth)
{
    if ((iType == eTYPE_CODE_STRING ||
      iType == eTYPE_CODE_TEXT) && (newWidth > iWidth))
    {
#ifdef VLAD_LOG_SEPARATION
        if (_verbose)
            _log << "Updating max width for " << tableName.c_str() << " " <<
              columnName << " to " << newWidth << endl;
#endif
        if (newWidth >= 10 && newWidth < 80)
        {
            newWidth = newWidth + 1;
        }
        else if (newWidth >= 80 && newWidth < 128)
        {
            newWidth = 128;
        }
        else if (newWidth >= 128 && newWidth < 255)
        {
            newWidth = 255;
        }
        else if (newWidth >= 255 && newWidth < 511)
        {
            newWidth = 511;
        }
        else if (newWidth >= 512 && newWidth < 1023)
        {
            newWidth = 1023;
        }
        else if (newWidth >= 1024 && newWidth < 4095)
        {
            newWidth = 4095;
        }
        else if (newWidth >= 4096 && newWidth < 16383)
        {
            newWidth = 16383;
        }
        else if (newWidth >= 16384 && newWidth < 32000)
        {
            newWidth = 32000;
        }
        else if (newWidth >= 32001 && newWidth < 64000)
        {
            newWidth = 64000;
        }

        string width = String::IntToString(newWidth);

        _set_attribute_value_where(_attribSchema, tableName, columnName,
          "width", width);
    }

    _set_attribute_value_where(_attribSchema, tableName, columnName,
      "populated", "Y");
}


void SchemaMap::CreateTables(CifFile& fobj, const string& blockName)
{

    // Create tables in the target schema ...

    vector<string> tNames;
    GetAllTablesNames(tNames);

    for (unsigned int i = 0; i < tNames.size(); ++i)
    {
        vector<string> cNames;
        GetAttributeNames(cNames, tNames[i]);

        if (cNames.empty())
        {
            continue;
        }

        ISTable* t = new ISTable(tNames[i]);
        for (unsigned int j = 0; j < cNames.size(); ++j)
        {
            t->AddColumn(cNames[j]);
        }

        Block& block = fobj.GetBlock(blockName);
        block.WriteTable(t);
    }

}


const vector<AttrInfo>& SchemaMap::GetTableAttributeInfo(
  const string& tableName, const vector<string>& columnsNames, 
  const Char::eCompareType compareType)
{
    if (tableName.empty())
    {
        throw EmptyValueException("Empty table name",
          "SchemaMap::GetTableAttributeInfo");
    }

    if (columnsNames.empty())
    {
        throw EmptyValueException("Empty column names",
          "SchemaMap::GetTableAttributeInfo");
    }

    if ((tableName == _tableName) && (columnsNames == _columnsNames) &&
      (compareType == _compareType))
    {
        return (_aI);
    }

    _aI.clear();

    AttrInfo tmpAttrInfo;
    tmpAttrInfo.iIndex     = false;
    tmpAttrInfo.iNull      = -1;
    tmpAttrInfo.iWidth     = -1;
    tmpAttrInfo.iPrecision = -1;
    tmpAttrInfo.iTypeCode  = eTYPE_CODE_NONE;

    _aI.insert(_aI.begin(), columnsNames.size(), tmpAttrInfo);

    // Get all of the attributes for this table.
    const vector<AttrInfo>& attrInfo = GetAttributesInfo(tableName);

    _columnsNames = columnsNames;
    _compareType = compareType;

    if (attrInfo.empty())
    {
#ifdef VLAD_LOG_SEPARATION
        if (_verbose)
            _log << "GetTableAttributeInfo: No attributes for found for " <<
              tableName << endl;
#endif
        throw EmptyValueException("Empty attribute info for category \"" +
          tableName + "\"", "SchemaMap::GetTableAttributeInfo");
    }

#ifdef VLAD_LOG_SEPARATION
    if (_verbose)
        _log << "GetTableAttributeInfo: +++ TABLE " << tableName << endl << endl;
#endif

    unsigned int nattrib = 0;
    for (unsigned int i = 0; i < attrInfo.size(); ++i)
    {
        const string& columnName = attrInfo[i].attribName;

#ifdef VLAD_LOG_SEPARATION
        if (_verbose)
            _log << "GetTableAttributeInfo: Column [" << j << "] " <<
              columnName << endl;
#endif
        try
        {
            int j = SchemaMap::GetTableColumnIndex(columnsNames, columnName,
              compareType);

            _aI[j] = attrInfo[i];

#ifdef VLAD_LOG_SEPARATION
	    if (_verbose)
                _log << " Width " << _aI[j].iWidth << " Type  " <<
                  _aI[j].iTypeCode << " Index " << _aI[j].iIndex << " Null " <<
                  _aI[j].iNull << endl;
#endif
            nattrib++;
        }
        catch (NotFoundException& exc)
        {
            // VLAD - Add some debugging, warning, reporting code
        }
    }

    if (nattrib < columnsNames.size())
    {
#ifdef VLAD_LOG_SEPARATION
        if (_verbose)
            _log << "GetTableAttributeInfo(): warning unmapped column nattrib = " <<
              nattrib  << endl;
#endif
    }

#ifdef VLAD_LOG_SEPARATION
    if (_verbose)
        _log << "GetTableAttributeInfo: +++ DONE WITH " << tableName << endl <<
          endl;
#endif

    return (_aI);
}


bool SchemaMap::AreValuesValid(const vector<string>& row,
  const vector<AttrInfo>& aI)
{
    if (aI.empty() || row.empty())
        return(false);

    for (unsigned int i = 0; i < row.size(); ++i)
    {
        if ((aI[i].iNull == 0) && (!aI[i].iIndex))
            continue;

#ifndef VLAD_HACK_DOT_VALUES
        if (CifString::IsEmptyValue(row[i]))
#else
        if (row[i] == CifString::UnknownValue)
#endif
            return(false);
    }

    return(true);
}


bool SchemaMap::IsTablePopulated(const string& tableName)
{

    const vector<AttrInfo>& attrInfo = GetAttributesInfo(tableName);

    if (attrInfo.empty())
    {
        return (false);
    }

    for (unsigned int i = 0; i < attrInfo.size(); ++i)
    {
        if (attrInfo[i].populated == "Y")
            return (true);
    }

    return (false);

}


ISTable* SchemaMap::CreateTableInfo() 
{

    ISTable* tinfo = NULL;

    const vector<string>& columnNames = _tableSchema->GetColumnNames();

    vector<string> tableNames;
    _tableSchema->GetColumn(tableNames, columnNames[0]);
    vector<string> c1;
    _tableSchema->GetColumn(c1, columnNames[1]);
    vector<string> c2;
    _tableSchema->GetColumn(c2, columnNames[2]);
    vector<string> c3;
    _tableSchema->GetColumn(c3, columnNames[3]);
    vector<string> c4;
    _tableSchema->GetColumn(c4, columnNames[4]);

    Block& rsBlock = _fobjS->GetBlock("rcsb_schema");
    _tableDescr = rsBlock.GetTablePtr("rcsb_table_description");

    tinfo = new ISTable("rcsb_tableinfo");

    tinfo->AddColumn("tablename");
    tinfo->AddColumn("description");
    tinfo->AddColumn("type");
    tinfo->AddColumn("table_serial_no");
    tinfo->AddColumn("group_name");
    tinfo->AddColumn("WWW_Selection_Criteria");  
    tinfo->AddColumn("WWW_Report_Criteria");  

    for (unsigned int i = 0; i < tableNames.size(); ++i)
    {
        if ((tableNames[i] == "tableinfo") || (tableNames[i] == "columninfo"))
            continue;
    
#ifdef VLAD_LOG_SEPARATION
        if (_verbose) _log << endl << "_CreateTableInfo  table = " <<
          tableNames[i] << endl;
#endif
    
        string cs3;

        vector<string> targVal;
        targVal.push_back(tableNames[i]);

        vector<string> targCol;
        targCol.push_back(columnNames[0]);

        unsigned int searchIndex = _tableDescr->FindFirst(targVal, targCol);
        if (searchIndex != _tableDescr->GetNumRows())
        {
	    cs3 = (*_tableDescr)(searchIndex, "description");
        }
        else
        {
#ifdef VLAD_LOG_SEPARATION
            if (_verbose)
                _log << endl << "_CreateTableInfo  NO information for " <<
                  tableNames[i] << endl;
#endif
            // Add default text 
            cs3 = "Missing table description";
        }

        vector<string> newRow;

        string cs2 = String::IntToString(i + 1);

        string tableNameDb;
        GetTableNameAbbrev(tableNameDb, tableNames[i]);
        newRow.push_back(tableNameDb);
        newRow.push_back(cs3);
        newRow.push_back(c1[i]);
        newRow.push_back(cs2);
        newRow.push_back(c2[i]);
        newRow.push_back(c3[i]);
        newRow.push_back(c4[i]);

        tinfo->AddRow(newRow);
    }

    return(tinfo);

}


ISTable* SchemaMap::CreateColumnInfo() 
{
  string tableNameDb, columnNameDb;

  ISTable* cinfo = NULL;

  const vector<string>& columnNames = _attribSchema->GetColumnNames();

  vector<string> c0;
  _attribSchema->GetColumn(c0, columnNames[0]);
  vector<string> c1;
  _attribSchema->GetColumn(c1, columnNames[1]);

  Block& rsBlock = _fobjS->GetBlock("rcsb_schema");
  _attribDescr = rsBlock.GetTablePtr("rcsb_attribute_description");

  _attribView = rsBlock.GetTablePtr("rcsb_attribute_view");

  cinfo = new ISTable("rcsb_columninfo");

  cinfo->AddColumn("tablename");
  cinfo->AddColumn("columnname");
  cinfo->AddColumn("description");
  cinfo->AddColumn("example");
  cinfo->AddColumn("type");
  cinfo->AddColumn("table_serial_no");
  cinfo->AddColumn("column_serial_no");
  cinfo->AddColumn("WWW_Selection_Criteria");  
  cinfo->AddColumn("WWW_Report_Criteria");  

  vector<string> q0;
  vector<string> iq0;
  
  int it=0;
  string cs2, cs3, cs4, cs5, cs6,cs7, cs8;
  
  vector<string> newRow;

  for (int i=0; i < (int) c0.size(); i++) {
    if (!strcmp(c0[i].c_str(),"tableinfo") || 
	!strcmp(c0[i].c_str(),"columninfo")) continue;
    if (i > 0 && strcmp(c0[i].c_str(),c0[i-1].c_str())) it++;
    cs8.clear(); cs8 += String::IntToString(it+1);
    cs2.clear(); cs2 += String::IntToString(i+1);
    cs3.clear();
    cs4.clear();  cs5.clear(); cs6.clear(); cs7.clear();

    q0.clear();
    q0.push_back(c0[i]);
    q0.push_back(c1[i]);
    iq0.clear();
    iq0.push_back("table_name");
    iq0.push_back("attribute_name");

    unsigned int searchIndex = _attribDescr->FindFirst(q0, iq0);
    if (searchIndex != _attribDescr->GetNumRows())
    {
	cs3 = (*_attribDescr)(searchIndex, "description");
	cs4 = (*_attribDescr)(searchIndex, "example");
    }
    else
    {
        cs3 = "Missing column description";
        cs4 = "Missing column example";
    }

    q0.clear();
    q0.push_back(c0[i]);
    q0.push_back(c1[i]);
    iq0.clear();
    iq0.push_back("table_name");
    iq0.push_back("attribute_name");
    searchIndex = _attribView->FindFirst(q0, iq0);
    if (searchIndex != _attribView->GetNumRows())
    {
	cs5 = (*_attribView)(searchIndex, "format_type");
	cs6 = (*_attribView)(searchIndex, "www_select");
	cs7 = (*_attribView)(searchIndex, "www_report");
    }
    else
    {
      cs5 = "3";
      cs6 = "1";
      cs7 = "1";
    }


    newRow.clear();

    GetTableNameAbbrev(tableNameDb, c0[i]);
    newRow.push_back(tableNameDb);

    GetAttributeNameAbbrev(columnNameDb, c0[i], c1[i]);
    newRow.push_back(columnNameDb);

    newRow.push_back(cs3); // description
    newRow.push_back(cs4); // example
    newRow.push_back(cs5);  // type

    newRow.push_back(cs8);  // column serial
    newRow.push_back(cs2);  // column serial
    newRow.push_back(cs6);  // www select
    newRow.push_back(cs7);  // www report

    cinfo->AddRow(newRow); 

  }

  return(cinfo);
}

void SchemaMap::GetMasterIndexAttribName(string& masterIndexAttribName)
{

    // Master index attribute name must be in the first row of
    // "rcsb_attribute_def" table, in "attribute_name" column

    masterIndexAttribName = (*_attribSchema)(0, "attribute_name");

}


unsigned int SchemaMap::GetTableColumnIndex(const vector<string>& colNames,
  const string& colName, const Char::eCompareType compareType)
{

    if (colName.empty())
    {
        // Empty column name.
        throw EmptyValueException("Empty column name",
          "SchemaMap::GetTableColumnIndex");
    }

    unsigned int colIndex = 0;

    for (colIndex = 0; colIndex < colNames.size(); ++colIndex)
    {
        if (String::IsEqual(colNames[colIndex], colName, compareType))
        {
            break;
        }
    }

    if (colIndex == colNames.size())
    {
        // Column not found.
        throw NotFoundException("Column not found",
          "SchemaMap::GetTableColumnIndex");
    }
    else
    {
        return(colIndex);
    }

}


static void _set_attribute_value_where(ISTable* t, const string& tableName,
  const string& attribName, const string& attributeChange,
  const string& valueChange)

{

    if ((t == NULL) || attribName.empty() || tableName.empty() ||
      attributeChange.empty() || valueChange.empty())
        return;

    if (!(t->IsColumnPresent(attributeChange)))
    {
        cout << "bad attribute in name" << endl;
        return;
    }

    vector<unsigned int> ir;
    vector<string> qcol, qval;    

    qcol.push_back("table_name");
    qcol.push_back("attribute_name");

    qval.push_back(tableName);
    qval.push_back(attribName);

    t->Search(ir, qval, qcol);

    if (!ir.empty())
    {
        t->UpdateCell(ir[0], attributeChange, valueChange);
    }

}

eTypeCode SchemaMap::_ConvertDataType(const string& dataType)
{
    if (String::IsCiEqual(dataType, "char"))
    {
        return(eTYPE_CODE_STRING);
    }
    else if (String::IsCiEqual(dataType, "int"))
    {
        return(eTYPE_CODE_INT);
    }
    else if (String::IsCiEqual(dataType, "integer"))
    {
        return(eTYPE_CODE_INT);
    }
    else if (String::IsCiEqual(dataType, "float"))
    {
        return(eTYPE_CODE_FLOAT);
    }
    else if (String::IsCiEqual(dataType, "double"))
    {
        return(eTYPE_CODE_FLOAT);
    }
    else if (String::IsCiEqual(dataType, "text"))
    {
        return(eTYPE_CODE_TEXT);
    }
    else if (String::IsCiEqual(dataType, "datetime"))
    {
        return(eTYPE_CODE_DATETIME);
    }
    else if (String::IsCiEqual(dataType, "date"))
    {
        return(eTYPE_CODE_DATETIME);
    }
    else if (String::IsCiEqual(dataType, "bigint"))
    {
        return(eTYPE_CODE_BIGINT);
    }
    else if (String::IsCiEqual(dataType, "varchar"))
    {
        return(eTYPE_CODE_TEXT);
    }
    else
    {
        cout << "Invalid data type: \"" << dataType << "\"" << endl;
        throw out_of_range("Invalid data type in SchemaMap::_ConvertDataType");
    }
}
