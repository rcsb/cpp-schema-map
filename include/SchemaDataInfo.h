//$$FILE$$
//$$VERSION$$
//$$DATE$$
//$$LICENSE$$


/**
** \file SchemaDataInfo.h
**
** Schema data info class.
*/


#ifndef SCHEMADATAINFO_H
#define SCHEMADATAINFO_H


#include <string>
#include <vector>

#include "rcsb_types.h"
#include "GenString.h"
#include "SchemaMap.h"
#include "DictObjCont.h"
#include "DictDataInfo.h"


class SchemaDataInfo : public DictDataInfo
{
  public:
    SchemaDataInfo(SchemaMap& schemaMap, const DictObjCont& dictObjCont);
    ~SchemaDataInfo();

    void GetVersion(std::string& version);

    const std::vector<std::string>& GetCatNames();

    const std::vector<std::string>& GetItemsNames();

    bool IsCatDefined(const std::string& catName) const;

    bool IsItemDefined(const std::string& itemName);

    const std::vector<std::string>& GetCatKeys(const std::string& catName);

    const std::vector<std::string>&
      GetItemAttribute(const std::string& itemName,
      const std::string& refCatName, const std::string& refAttribName);

    bool AreAllKeyItems(const std::string& catName,
      const std::vector<std::string>& attribsNames);

    bool IsUnknownValueAllowed(const std::string& catName,
      const std::string& attribName);

    bool IsKeyItem(const std::string& catName, const std::string& attribName,
      const Char::eCompareType compareType = Char::eCASE_SENSITIVE);

    void GetItemsTypes(std::vector<eTypeCode>& attribsTypes,
      const std::string& catName, const std::vector<std::string>& attribsNames);

    void StandardizeEnumItem(std::string& value, const std::string& catName,
      const std::string& attribName);

    bool IsItemMandatory(const std::string& itemName);
    bool IsSimpleDataType(const std::string& itemName);
    eTypeCode _GetDataType(const std::string& itemName);

    void GetCatItemsNames(std::vector<std::string>& itemsNames,
      const std::string& catName);

    void GetCatAttribsNames(std::vector<std::string>& attribsNames,
      const std::string& catName);

    void GetParentCifItems(std::vector<std::string>& parCifItems,
      const std::string& cifItemName);

  private:
    SchemaMap& _schemaMap;

    std::vector<std::string> _catNames;
    std::vector<std::string> _itemNames;

    std::string _catName;
    std::vector<std::string> _catKeys;
    std::vector<AttrInfo> _attrInfo;

    std::vector<std::string> _itemAttrib;


    bool _IsSpecialAttribute(const std::string& attribName);
    void _GetCatAttribsInfo(const std::string& catName);
};


#endif
