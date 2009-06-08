//$$FILE$$
//$$VERSION$$
//$$DATE$$
//$$LICENSE$$


/*!
** \file SchemaParentChild.h
**
** \brief Header file for SchemaParentChild class.
*/


#ifndef SCHEMAPARENTCHILD_H
#define SCHEMAPARENTCHILD_H


#include "DictObjCont.h"
#include "DictParentChild.h"
#include "SchemaDataInfo.h"


class SchemaParentChild : public DictParentChild
{
  public:
    SchemaParentChild(SchemaDataInfo& schemaDataInfo,
      const DictObjCont& dictObjCont);
    virtual ~SchemaParentChild();
};


#endif

