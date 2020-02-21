#pragma once

#include "../../../Common/Common/common.h"
using namespace common;
#include "../../../Common/Graphic11/graphic11.h"
#include "../../../Common/Framework11/framework11.h"
#include "../../../Common/Physics/physics.h"


#include "creature/evc.h"
#include "creature/genotype.h"
#include "creature/generator.h"
#include "creature/effector.h"
#include "creature/sensor.h"
#include "creature/angularsensor.h"
#include "creature/muscleeffector.h"
#include "creature/neuralnet.h"
#include "creature/gnode.h"
#include "creature/pnode.h"
#include "creature/glink.h"
#include "lib/jointrenderer.h"
#include "lib/phenotypemanager.h"
#include "lib/genotypemanager.h"
#include "global.h"


extern cGlobal *g_global;
extern cPhenoTypeManager *g_pheno;
extern cGenoTypeManager *g_geno;
extern framework::cGameMain2 *g_application;
