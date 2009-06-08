/*$$FILE$$*/
/*$$VERSION$$*/
/*$$DATE$$*/
/*$$LICENSE$$*/


/*!
** \file SchemaMap.h
**
** \brief Header file for SchemaMap related classes.
*/


#ifndef SCHEMAMAP_H
#define SCHEMAMAP_H


#include <string>

#include "GenString.h"
#include "CifFile.h"
#include "DicFile.h"

typedef struct
{
    string attribName;
    string dataType;
    eTypeCode iTypeCode; // from dataType
    bool iIndex;
    int iNull;
    int iWidth;
    int iPrecision;
    string populated;   // Not taken from the file, but constructed afterwards
} AttrInfo;


/**
**  \class SchemaMap
**
**  \brief Public class that respresents schema mapping configuration.
**
**  This class encapsulates schema mapping functionality. Schema map
**  defines how mmCIF data gets converted into the database data. Schema
**  map information is stored in a number of configuration tables
**  that are located in a configuration CIF file. This class provides methods
**  for reading the schema mapping information from the ASCII CIF file or
**  serialized (binary) configuration CIF file, for revising the schema
**  mapping information (based on the data), updating the schema mapping
**  information, accessing the attributes information.
*/
class SchemaMap
{

  public:
    static string _DATABLOCK_ID;
    static string _DATABASE_2;
    static string _ENTRY_ID;

    static const int _MAX_LINE_LENGTH = 255;

    static string _STRUCTURE_ID_COLUMN;

    /**
    **  Constructs a schema mapping object.
    **  
    **  \param[in] schemaFile - optional parameter that indicates the name of
    **    the ASCII CIF file, which holds the schema mapping configuration
    **    information. If specified, \e schemaFile file is parsed and the
    **    schema mapping configuration information is taken from it.
    **  \param[in] schemaFileOdb - optional parameter that indicates the name
    **    of the serialized CIF file, which holds the schema mapping
    **    configuration information. If \e schemaFile is not specified,
    **    \e schemaFileOdb is de-serialized and the schema mapping
    **    configuration information is taken from it. If both \e schemaFile
    **    and \e schemaFileOdb are specified, \e schemaFile is parsed,
    **    the schema mapping configuration information is taken from it and
    **    then serialized to \e schemaFileOdb file, at the end of processing.
    **  \param[in] verbose - optional parameter that indicates whether
    **    logging should be turned on (if true) or off (if false).
    **    If \e verbose is not specified, logging is turned off.
    **
    **  \return Not applicable
    **
    **  \pre \e schemaFile and \e schemaFileOdb cannot both be empty.
    **
    **  \post None
    **
    **  \exception: None
    */
    SchemaMap(const string& schemaFile = std::string(),
      const string& schemaFileOdb = std::string(), bool verbose = false);

    SchemaMap(const eFileMode fileMode, const string& dictSdbFileName,
      const bool verbose);

    void Create(const string& op, const string& inFile,
      const string& structId);

    void Write(const string& fileName);

    /**
    **  Destructs a schema mapping object, by releasing all the used resources.
    **
    **  \param: Not applicable
    **
    **  \return Not applicable
    **
    **  \pre None
    **
    **  \post None
    **
    **  \exception: None
    */
    virtual ~SchemaMap();

    /**
    **  Sets the schema revising mode. Schema revising is a process where
    **  the existing schema mapping information is being updated, by examining
    **  the attributes of the converted mmCIF file. Schema revising is needed
    **  in order to increase the length of database attributes in database
    **  schema/data file so that the data can be fully stored in the database.
    **
    **  \param[in] mode - optional parameter that indicates whether
    **    the schema revising should be turned on (if true) or off (if false).
    **    If \e mode is not specified, schema revising is turned on.
    **
    **  \return None
    **
    **  \pre None
    **
    **  \post None
    **
    **  \exception: None
    */
    void SetReviseSchemaMode(bool mode = true);

    /**
    **  Retrieves schema revising mode.
    **
    **  \param: None
    **
    **  \return true - if schema revising is turned on
    **  \return false - if schema revising is turned off
    **
    **  \pre None
    **
    **  \post None
    **
    **  \exception: None
    */
    bool GetReviseSchemaMode();

    /**
    **  Writes the revised schema mapping information to a file.
    **  
    **  \param[in] revisedSchemaFile - indicates the name of
    **    the ASCII CIF file, to which the revised schema mapping information
    **    will be written to.
    **
    **  \return None
    **
    **  \pre None
    **
    **  \post None
    **
    **  \exception: None
    */
    void ReviseSchemaMap(const string& revisedSchemaFile);


    /**
    **  Updates the existing schema mapping object, with the revised
    **  information from the specified schema mapping object.
    **
    **  \param[in] revSchMap - reference to a revised schema mapping object.
    **
    **  \return None
    **
    **  \pre None
    **
    **  \post None
    **
    **  \exception: None
    */
    void updateSchemaMapDetails(SchemaMap& revSchMap);

    /**
    **  In the specified CIF file object, creates tables that are specified in
    **  the schema mapping object.
    **
    **  \param[in] cifFile - reference to a CIF file object that will hold
    **    the data.
    **  \param[in] blockName - the name of the data block
    **
    **  \return None
    **
    **  \pre None
    **
    **  \post None
    **
    **  \exception: None
    */
    void CreateTables(CifFile& cifFile, const string& blockName);

    /**
    **  Retrieves names of all the tables in each data block, defined in
    **  schema mapping object.
    **
    **  \param[out] tablesNames - retrieved tables names
    **
    **  \return None
    **
    **  \pre None
    **
    **  \post None
    **
    **  \exception: None
    */
    void GetAllTablesNames(vector<string>& tablesNames);
    void GetDataTablesNames(vector<string>& tablesNames);

    /**
    **  Utility method, not part of users public API.
    */
    static unsigned int GetTableColumnIndex(const vector<string>& columnsNames,
      const string& colName, const Char::eCompareType compareType =
      Char::eCASE_SENSITIVE);

    ISTable* CreateTableInfo();
    ISTable* CreateColumnInfo();

    void GetAttributeNames(vector<string>& attributes,
      const string& tableName);
    void GetMappedAttributesInfo(vector<vector<string> >& mappedAttrInfo,
      const string& tableName);

    void GetMappedConditions(vector<vector<string> >& mappedConditions,
      const string& tableName);

    void GetTableNameAbbrev(string& abbrevTableName, const string& tableName);
    void GetAttributeNameAbbrev(string& abbrevAttrName, const string& tableName,
      const string& attribName);

    void getSchemaMapDetails(const string& tableName,
      const string& attribName, string& width, string& populated);

    void UpdateAttributeDef(const string& tableName, const string& columnName,
      int type, int iWidth, int newWidth);

    const vector<AttrInfo>& GetTableAttributeInfo(const string& tableName,
      const vector<string>& columnsNames,
      const Char::eCompareType compareType);

    const vector<AttrInfo>& GetAttributesInfo(const string& tableName);

    static bool AreValuesValid(const vector<string>& row,
      const vector<AttrInfo>& aI);
    bool IsTablePopulated(const string& tableName);
    void GetMasterIndexAttribName(string& masterIndexAttribName);

  private:
    bool _verbose;

    string _schemaFile;    // Schema definition file name (mmCIF)
    string _schemaFileOdb; // Schema definition file name (odb)

    CifFile* _fobjS;

    ISTable* _tableSchema;
    ISTable* _attribSchema;
    ISTable* _schemaMap;
    ISTable* _mapConditions;
    ISTable* _tableAbbrev;    
    ISTable* _attribAbbrev;

    ISTable* _attribDescr;
    ISTable* _tableDescr;
    ISTable* _attribView;

    bool _reviseSchemaMode;
    int  _compatibilityMode;

    DicFile* _dict;

    ISTable* _itemTypeTab;
    ISTable* _itemLinkedTab;
    ISTable* _itemDescriptionTab;
    ISTable* _itemExamplesTab;
    ISTable* _categoryTab;
    ISTable* _categoryGroupTab;
    ISTable* _categoryKeyTab;
    ISTable* _itemTab;
    ISTable* _itemAliasesTab;

    string _tableName;
    vector<string> _columnsNames;
    Char::eCompareType _compareType;
    vector<AttrInfo> _aI;
    vector<AttrInfo> _attrInfo;

    void _AssignAttribIndices(void);
    void GetGroupNames(vector<string>& groupNames);

    void Clear();

    void UpdateTableDescr(const string& category);
    void UpdateTableSchema(const string& category);
    void MapAlias(const string& tableName, const string& colName,
      const string& op);
    void GetItemDescription(string& itemDescription,
      const string& itemName);
    void GetFirstItemExample(string& firstItemExample,
      const string& itemName);
    void GetTopParentName(string& topParentName, const string& itemName);
    void GetTypeCode(string& typeCode, const string& topParentName,
      const string& itemName);
    void GetCategoryKeys(vector<string>& categoryKeys,
      const string& category);
    void CreateMappingTables();
    void WriteMappingTables();
    void GetTablesAndColumns(vector<string>& tableList,
      vector<vector<string> >& allColNames, const string& op,
      const string& inFile);
    void UpdateMapConditions();

    void GetDictTables(); 
    void GetColumnsFromDict(vector<string>& colNames,
      const string& tableName);

    void AbbrevTableName(string& dbTableNameAbbrev, const string& tableName);
    void AbbrevColName(const string& tableName, const string& colName);

    void UpdateAttribSchema(const string& tableName, const string& colName,
      const unsigned int itype, const vector<string>& categoryKeys);
    void UpdateAttribView(const string& tableName, const string& colName,
      const unsigned int itype);
    void UpdateAttribDescr(const string& tableName, const string& colName);

    void UpdateSchemaMap(const string& tableName, const string& colName,
      const string& structId);

    unsigned int GetTypeCodeIndex(const string& tableName,
      const string& colName);

    eTypeCode _ConvertDataType(const string& dataType);
};

#endif
