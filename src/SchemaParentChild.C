//$$FILE$$
//$$VERSION$$
//$$DATE$$
//$$LICENSE$$


/*!
** \file SchemaParentChild.C
**
** \brief Implementation file for SchemaParentChild class.
*/


#include <string>
#include <vector>

#include "GenString.h"
#include "CifString.h"
#include "GenCont.h"
#include "ISTable.h"
#include "TableFile.h"
#include "DictParentChild.h"
#include "SchemaParentChild.h"

using std::string;
using std::vector;


SchemaParentChild::SchemaParentChild(SchemaDataInfo& schemaDataInfo,
  const DictObjCont& dictObjCont) : DictParentChild(dictObjCont, schemaDataInfo)
{

}


SchemaParentChild::~SchemaParentChild()
{

}


