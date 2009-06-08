//$$FILE$$
//$$VERSION$$
//$$DATE$$
//$$LICENSE$$


#include <string>
#include <vector>

#include "rcsb_types.h"
#include "GenCont.h"
#include "DictObjCont.h"
#include "SchemaDataInfo.h"


using std::string;
using std::vector;


SchemaDataInfo::SchemaDataInfo(SchemaMap& schemaMap,
  const DictObjCont& dictObjCont) : DictDataInfo(dictObjCont),
  _schemaMap(schemaMap)
{
    _schemaMap.GetDataTablesNames(_catNames);

    for (unsigned int i = 0; i < _catNames.size(); ++i)
    {
        vector<string> attribNames;
        _schemaMap.GetAttributeNames(attribNames, _catNames[i]);

        for (unsigned int j = 0; j < attribNames.size(); ++j)
        {
            string cifItem;
            CifString::MakeCifItem(cifItem, _catNames[i], attribNames[j]);

            _itemNames.push_back(cifItem);
        }
    }
}


SchemaDataInfo::~SchemaDataInfo()
{

}


void SchemaDataInfo::GetVersion(string& version)
{
    version = "1.0";
}


const vector<string>& SchemaDataInfo::GetCatNames()
{
    return (_catNames);
}


const vector<string>& SchemaDataInfo::GetItemsNames()
{
    return (_itemNames);
}


bool SchemaDataInfo::IsCatDefined(const string& catName) const
{
    return (GenCont::IsInVector(catName, _catNames));
}


bool SchemaDataInfo::IsItemDefined(const string& itemName)
{
    return (GenCont::IsInVector(itemName, _itemNames));
}


const vector<string>& SchemaDataInfo::GetCatKeys(const string& catName)
{
    if ((catName == _catName) && !_catKeys.empty())
        return (_catKeys);

    _GetCatAttribsInfo(catName);

    for (unsigned int i = 0; i < _attrInfo.size(); ++i)
    {
        if (_attrInfo[i].iIndex)
        {
            string cifItem;
            CifString::MakeCifItem(cifItem, catName, _attrInfo[i].attribName);

            _catKeys.push_back(cifItem);
        }
    }

    return (_catKeys);
}


const vector<string>& SchemaDataInfo::GetItemAttribute(const string& itemName,
  const string& refCatName, const string& refAttribName)
{
    string attribName;
    CifString::GetItemFromCifItem(attribName, itemName);
 
    if (!_IsSpecialAttribute(attribName))
    {
        const ObjCont& itemCont = _dictObjCont.GetObjCont(itemName,
          ItemObjContInfo::GetInstance());

        return (itemCont.GetAttribute(refCatName, refAttribName));
    }
    else
    {
        return (_itemAttrib);
    }
}


bool SchemaDataInfo::AreAllKeyItems(const string& catName,
  const vector<string>& attribsNames)
{
    const vector<AttrInfo>& aI = _schemaMap.GetTableAttributeInfo(catName,
      attribsNames, Char::eCASE_SENSITIVE);

    for (unsigned int j = 0; j < aI.size(); ++j)
    {
        if (!aI[j].iIndex)
        {
            return (false);
        }
    }

    return (true);
}


bool SchemaDataInfo::IsUnknownValueAllowed(const string& catName,
  const string& attribName)
{
    vector<string> attribsNames;
    attribsNames.push_back(attribName);

    const vector<AttrInfo>& aI = _schemaMap.GetTableAttributeInfo(catName,
      attribsNames, Char::eCASE_SENSITIVE);

    return (!aI[0].iNull);
}


bool SchemaDataInfo::IsKeyItem(const string& catName, const string& attribName,
  const Char::eCompareType compareType)
{
    vector<string> attribsNames;
    GetCatAttribsNames(attribsNames, catName);

    const vector<AttrInfo>& aI = _schemaMap.GetTableAttributeInfo(catName,
      attribsNames, compareType);

    unsigned int j = SchemaMap::GetTableColumnIndex(attribsNames, attribName,
      compareType);

    return (aI[j].iIndex);
}


void SchemaDataInfo::GetItemsTypes(vector<eTypeCode>& attribsTypes,
  const string& catName, const vector<string>& attribsNames)
{
    attribsTypes.clear();

    // Figure out the special attributes and remove them from attribsNames
    vector<string> dictAttribsNames;
    for (unsigned int attribI = 0; attribI < attribsNames.size(); ++attribI)
    {
        if (!_IsSpecialAttribute(attribsNames[attribI]))
        {
            dictAttribsNames.push_back(attribsNames[attribI]);
        }
    }
      
    // Get types for them from DictDataInfo::GetItemsTypes
    vector<eTypeCode> dictAttribsTypes;
    DictDataInfo::GetItemsTypes(dictAttribsTypes, catName, dictAttribsNames);

    for (unsigned int attribI = 0, dictAttribI = 0;
      attribI < attribsNames.size(); ++attribI)
    {
        if (!_IsSpecialAttribute(attribsNames[attribI]))
        {
            attribsTypes.push_back(dictAttribsTypes[dictAttribI]);
            ++dictAttribI;
        }
        else
            // VLAD - THIS REQUIRES PROPER HANDLING IN CASE THERE ARE MORE
            // SPECIAL ATTRIBUTES THAN Structure_ID
            attribsTypes.push_back(eTYPE_CODE_STRING);
    }
}


void SchemaDataInfo::StandardizeEnumItem(string& value,
  const string& catName, const string& attribName)
{

}


bool SchemaDataInfo::IsItemMandatory(const string& itemName)
{
    string attribName;
    CifString::GetItemFromCifItem(attribName, itemName);
 
    if (!_IsSpecialAttribute(attribName))
    {
        return (DictDataInfo::IsItemMandatory(itemName));
    }
    else
    {
        return (true);
    }
}


bool SchemaDataInfo::IsSimpleDataType(const string& itemName)
{
    string attribName;
    CifString::GetItemFromCifItem(attribName, itemName);
 
    if (!_IsSpecialAttribute(attribName))
    {
        return (DictDataInfo::IsSimpleDataType(itemName));
    }
    else
    {
        return (true);
    }
}


eTypeCode SchemaDataInfo::_GetDataType(const string& itemName)
{
    string attribName;
    CifString::GetItemFromCifItem(attribName, itemName);

    if (!_IsSpecialAttribute(attribName))
    {
        return (DictDataInfo::_GetDataType(itemName));
    } 
    else
        // VLAD - THIS REQUIRES PROPER HANDLING IN CASE THERE ARE MORE
        // SPECIAL ATTRIBUTES THAN Structure_ID
        return (eTYPE_CODE_STRING);
}


void SchemaDataInfo::GetCatItemsNames(vector<string>& itemsNames,
  const string& catName)
{
    itemsNames.clear();

    const vector<AttrInfo>& attrInfo = _schemaMap.GetAttributesInfo(catName);

    for (unsigned int i = 0; i < attrInfo.size(); ++i)
    {
        string cifItem;
        CifString::MakeCifItem(cifItem, catName, attrInfo[i].attribName);

        itemsNames.push_back(cifItem);
    }
}


void SchemaDataInfo::GetCatAttribsNames(vector<string>& attribsNames,
  const string& catName)
{
    attribsNames.clear();

    const vector<AttrInfo>& attrInfo = _schemaMap.GetAttributesInfo(catName);

    for (unsigned int i = 0; i < attrInfo.size(); ++i)
    {
        attribsNames.push_back(attrInfo[i].attribName);
    }
}


void SchemaDataInfo::GetParentCifItems(vector<string>& parCifItems,
  const string& cifItemName)
{
    parCifItems.clear();

    string attribName;
    CifString::GetItemFromCifItem(attribName, cifItemName);

    if (attribName == SchemaMap::_STRUCTURE_ID_COLUMN) 
    {
        return;
    }

    const ObjCont& itemCont = _dictObjCont.GetObjCont(cifItemName,
      ItemObjContInfo::GetInstance());

    const vector<string>& allParCifItems =
      itemCont.GetAttribute(CifString::CIF_DDL_CATEGORY_ITEM_LINKED,
        CifString::CIF_DDL_ITEM_PARENT_NAME);

    // VLAD - improve algorithm - very poor
    for (unsigned int itemI = 0; itemI < allParCifItems.size(); ++itemI)
    {
        string parCatName;
        CifString::GetCategoryFromCifItem(parCatName, allParCifItems[itemI]);
        
        vector<string> definedParItems;
        GetCatItemsNames(definedParItems, parCatName);

        if (GenCont::IsInVector(allParCifItems[itemI], definedParItems))
            parCifItems.push_back(allParCifItems[itemI]);
    }
}


bool SchemaDataInfo::_IsSpecialAttribute(const string& attribName)
{
    if (attribName == SchemaMap::_STRUCTURE_ID_COLUMN)
    {
        return (true);
    }

    return (false);
}


void SchemaDataInfo::_GetCatAttribsInfo(const string& catName)
{
    if ((catName == _catName) && !_attrInfo.empty())
    {
        return;
    }

    // RESET CACHE
    _catName = catName;
    _catKeys.clear();
    _attrInfo.clear();

    _attrInfo = _schemaMap.GetAttributesInfo(_catName);
}

