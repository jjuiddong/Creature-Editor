
#include "stdafx.h"
#include "generator.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include "parser/GenotypeParser.h"


namespace evc
{
	Vector4 ParseVector4(const string &str);
	Vector3 ParseVector3(const string &str);
	Vector2 ParseVector2(const string &str);
	string Vector2ToStr(const Vector2 &vec);
	string Vector3ToStr(const Vector3 &vec);
	string Vector3ToStr(const string &name, const Vector3 &vec);
	string Vector4ToStr(const Vector4 &vec);
	string QuaternionToStr(const Quaternion &q);
	string TransformToStr(const Transform &tfm);
	string ConeLimitToStr(const sConeLimit &limit);
	string AngularLimitToStr(const sAngularLimit &limit);
	string LinearLimitToStr(const sLinearLimit &limit);
	string DistanceLimitToStr(const sDistanceLimit &limit);
	string DriveInfoToStr(const sDriveInfo &drive);

	bool Put_RigidActor(boost::property_tree::ptree &parent, phys::cRigidActor *actor);
	bool Put_Joint(boost::property_tree::ptree &parent, phys::cJoint *joint);
	bool Put_GNode(boost::property_tree::ptree &parent, evc::cGNode *gnode);
	bool Put_GLink(boost::property_tree::ptree &parent, evc::cGLink *glink);
}

using namespace evc;



// create cCreature from rigidactor
cCreature* evc::GenerateCreatureFrom_RigidActor( graphic::cRenderer &renderer
	, phys::cRigidActor *actor)
{
	if (!WritePhenoTypeFileFrom_RigidActor("tmp.pnt", actor))
		return false;

	cCreature *creature = ReadPhenoTypeFile(renderer, "tmp.pn");
	return creature;
}


// write phenotype node file
//
// {
//		"creature" : 
//		{
//			"shapes" :
//			[
//				"shape" :
//				{
//					"name" : "name",
//					"id" : "100",
//					"type" : "box",
//					"pos" : "10 10 10",
//					"dim" : "2 0.4 1",
//					"rot" : "0 0 0 1",
//					"mass" : "1.0",
//					"angular damping" : "1.0",
//					"linear damping" : "1.0",
//					"kinematic" : "0"
//				},
//				"shape" :
//				{
//	
//	
//				}
//			]
//		}
// }
//
bool evc::WritePhenoTypeFileFrom_RigidActor(const StrPath &fileName
	, phys::cRigidActor *actor)
{
	using namespace phys;

	// collect shape, joint
	set<cRigidActor*> actors;
	set<cJoint*> joints;
	queue<cRigidActor*> q;
	q.push(actor);
	while (!q.empty())
	{
		cRigidActor *a = q.front();
		q.pop();
		if (!a)
			continue;
		if (actors.end() != std::find(actors.begin(), actors.end(), a))
			continue; // already exist

		actors.insert(a);
		for (auto *j : a->m_joints)
		{
			q.push(j->m_actor0);
			q.push(j->m_actor1);
			joints.insert(j);
		}
	}

	// make ptree
	try
	{
		using boost::property_tree::ptree;
		ptree props;
		ptree creature;
		common::Str128 text;
		Vector3 v;
		Quaternion q;

		creature.put("version", "1.01");
		creature.put("name", "creature");

		// make shape
		ptree shapes;
		for (auto &a : actors)
			Put_RigidActor(shapes, a);

		// make joint
		ptree jts;
		for (auto &j : joints)
			Put_Joint(jts, j);

		creature.add_child("shapes", shapes);
		creature.add_child("joints", jts);
		props.add_child("creature", creature);
		boost::property_tree::write_json(fileName.c_str(), props);
	}
	catch (std::exception &e)
	{
		common::Str128 msg;
		msg.Format("Write Error!!, File [ %s ]\n%s"
			, fileName.c_str(), e.what());
		//MessageBoxA(NULL, msg.c_str(), "ERROR", MB_OK);
		return false;
	}

	return true;
}


bool evc::WritePhenoTypeFileFrom_RigidActor(const StrPath &fileName
	, const vector<phys::cRigidActor*> &actors)
{
	using namespace phys;

	set<cJoint*> joints;
	for (auto &a : actors)
	{
		for (auto *j : a->m_joints)
			joints.insert(j);
	}

	// make ptree
	try
	{
		using boost::property_tree::ptree;
		ptree props;
		ptree creature;
		common::Str128 text;
		Vector3 v;
		Quaternion q;

		creature.put("version", "1.01");
		creature.put("name", "creature");

		// make shape
		ptree shapes;
		for (auto &a : actors)
			Put_RigidActor(shapes, a);

		// make joint
		ptree jts;
		for (auto &j : joints)
		{
			// check exist actor?
			auto it0 = std::find(actors.begin(), actors.end(), j->m_actor0);
			auto it1 = std::find(actors.begin(), actors.end(), j->m_actor1);
			if ((actors.end() == it0) || (actors.end() == it1))
				continue; // ignore joint, if not exist actor
			Put_Joint(jts, j);
		}

		creature.add_child("shapes", shapes);
		creature.add_child("joints", jts);
		props.add_child("creature", creature);
		boost::property_tree::write_json(fileName.c_str(), props);
	}
	catch (std::exception &e)
	{
		common::Str128 msg;
		msg.Format("Write Error2!!, File [ %s ]\n%s"
			, fileName.c_str(), e.what());
		//MessageBoxA(NULL, msg.c_str(), "ERROR", MB_OK);
		return false;
	}

	return true;
}


// read phenotype node file and create cCreature
cCreature* evc::ReadPhenoTypeFile(graphic::cRenderer &renderer
	, const StrPath &fileName
	, OUT vector<int> *outSyncIds //= nullptr
)
{
	using boost::property_tree::ptree;

	map<int, phys::sSyncInfo*> syncs;

	try
	{
		ptree props;
		boost::property_tree::read_json(fileName.c_str(), props);

		// parse creature
		ptree::assoc_iterator itor0 = props.find("creature");
		if (props.not_found() != itor0)
		{
			if (props.not_found() != itor0->second.find("shapes"))
			{
				ptree &child_field0 = itor0->second.get_child("shapes");
				for (ptree::value_type &vt0 : child_field0)
				{
					const string name = vt0.second.get<string>("name", "blank");
					const int id = vt0.second.get<int>("id");
					const phys::eRigidType::Enum type = phys::eRigidType::FromString(
						vt0.second.get<string>("type", "Dynamic"));
					const phys::eShapeType::Enum shape = phys::eShapeType::FromString(
						vt0.second.get<string>("shape", "Box"));
					const string posStr = vt0.second.get<string>("pos", "0 0 0");
					const Vector3 pos = ParseVector3(posStr);
					const string dimStr = vt0.second.get<string>("dim", "1 1 1");
					const Vector3 dim = ParseVector3(dimStr);
					const string rotStr = vt0.second.get<string>("rot", "0 0 0 1");
					const Vector4 rot = ParseVector4(rotStr);
					const Quaternion q(rot.x, rot.y, rot.z, rot.w);
					const float mass = vt0.second.get<float>("mass", 0.f);
					const float angularDamping = vt0.second.get<float>("angular damping", 1.f);
					const float linearDamping = vt0.second.get<float>("linear damping", 1.f);
					const bool kinematic = vt0.second.get<bool >("kinematic", true);

					int newId = -1;
					switch (shape)
					{
					case phys::eShapeType::Plane:
						break;

					case phys::eShapeType::Box:
					{
						const Transform tfm(pos, dim, q);
						newId = g_evc->m_sync->SpawnBox(renderer
							, tfm, 1.f, true);
					}
					break;

					case phys::eShapeType::Sphere:
					{
						const Transform tfm(pos, dim, q);
						newId = g_evc->m_sync->SpawnSphere(renderer
							, tfm, dim.x, 1.f, true);
					}
					break;

					case phys::eShapeType::Capsule:
					{
						const Transform tfm(pos, q);
						newId = g_evc->m_sync->SpawnCapsule(renderer
							, tfm, dim.y, dim.x - dim.y, 1.f, true);
					}
					break;

					case phys::eShapeType::Cylinder:
					{
						const Transform tfm(pos, q);
						newId = g_evc->m_sync->SpawnCylinder(renderer
							, tfm, dim.y, dim.x, 1.f, true);
					}
					break;

					case phys::eShapeType::Convex:
					default:
						throw std::exception("shape type error");
					}

					if (phys::sSyncInfo *sync = g_evc->m_sync->FindSyncInfo(newId))
					{
						if (mass != 0.f)
							sync->actor->SetMass(mass);
						syncs[id] = sync;
					}
				}//~for shapes
			}//~shapes

			if (props.not_found() != itor0->second.find("joints"))
			{
				ptree &child_field0 = itor0->second.get_child("joints");
				for (ptree::value_type &vt0 : child_field0)
				{
					const phys::eJointType::Enum type = phys::eJointType::FromString(
						vt0.second.get<string>("type", "Fixed"));
					const int actorId0 = vt0.second.get<int>("shape id0");
					const int actorId1 = vt0.second.get<int>("shape id1");
					const string revoluteRotStr = vt0.second.get<string>("revolute rot", "0 0 0 1");
					const Vector4 rot = ParseVector4(revoluteRotStr);
					const Quaternion revoluteQ(rot.x, rot.y, rot.z, rot.w);
					const string pivotDirStr0 = vt0.second.get<string>("pivot dir0", "1 0 0");
					const string pivotDirStr1 = vt0.second.get<string>("pivot dir1", "1 0 0");
					const Vector3 pivotDir0 = ParseVector3(pivotDirStr0);
					const Vector3 pivotDir1 = ParseVector3(pivotDirStr1);
					const float pivotLen0 = vt0.second.get<float>("pivot len0");
					const float pivotLen1 = vt0.second.get<float>("pivot len1");

					auto it0 = syncs.find(actorId0);
					auto it1 = syncs.find(actorId1);
					if ((syncs.end() == it0) || (syncs.end() == it1))
						continue; // error but continue

					phys::sSyncInfo *sync0 = it0->second;
					phys::sSyncInfo *sync1 = it1->second;

					// GetPivotWorldTransform
					const Transform tfm0 = sync0->node->m_transform;
					const Transform tfm1 = sync1->node->m_transform;
					const Vector3 localPos0 = pivotDir0 * tfm0.rot * pivotLen0;
					const Vector3 pivot0 = tfm0.pos + localPos0;
					const Vector3 localPos1 = pivotDir1 * tfm1.rot * pivotLen1;
					const Vector3 pivot1 = tfm1.pos + localPos1;
					const Vector3 revoluteAxis = Vector3(1, 0, 0) * revoluteQ;

					switch (type)
					{
					case phys::eJointType::Fixed:
					{
						phys::cJoint *joint = new phys::cJoint();
						joint->CreateFixed(*g_evc->m_phys
							, sync0->actor, sync0->node->m_transform, pivot0
							, sync1->actor, sync1->node->m_transform, pivot1);

						cJointRenderer *jointRenderer = new cJointRenderer();
						jointRenderer->Create(*g_evc->m_sync, joint);
						g_evc->m_sync->AddJoint(joint, jointRenderer);
					}
					break;

					case phys::eJointType::Spherical:
					{
						const bool isConeLimit = vt0.second.get<bool>("cone limit", false);
						const string coneLimitStr = vt0.second.get<string>("cone limit config", "0 0 0.01");
						const Vector3 tconf0 = ParseVector3(coneLimitStr);
						physx::PxJointLimitCone coneLimit(tconf0.x, tconf0.y, tconf0.z);

						phys::cJoint *joint = new phys::cJoint();
						joint->CreateSpherical(*g_evc->m_phys
							, sync0->actor, sync0->node->m_transform, pivot0
							, sync1->actor, sync1->node->m_transform, pivot1);

						if (isConeLimit)
						{
							joint->EnableConeLimit(true);
							joint->SetConeLimit(coneLimit);
						}

						cJointRenderer *jointRenderer = new cJointRenderer();
						jointRenderer->Create(*g_evc->m_sync, joint);
						g_evc->m_sync->AddJoint(joint, jointRenderer);
					}
					break;

					case phys::eJointType::Revolute:
					{
						const bool isDrive = vt0.second.get<bool>("drive");
						const float driveVelocity = vt0.second.get<float>("drive velocity");
						const bool isCycle = vt0.second.get<bool>("cycle", false);
						const float cyclePeriod = vt0.second.get<float>("cycle period", 0.f);
						const float cycleAccel = vt0.second.get<float>("cycle accel", 0.f);

						const bool isAngularLimit = vt0.second.get<bool>("angular limit", false);
						const string angularLimitStr = vt0.second.get<string>("angular limit config", "0 0 0.01");
						const Vector3 tconf1 = ParseVector3(angularLimitStr);
						physx::PxJointAngularLimitPair angularLimit(tconf1.x, tconf1.y, tconf1.z);

						phys::cJoint *joint = new phys::cJoint();
						joint->CreateRevolute(*g_evc->m_phys
							, sync0->actor, sync0->node->m_transform, pivot0
							, sync1->actor, sync1->node->m_transform, pivot1
							, revoluteAxis);

						if (isAngularLimit)
						{
							joint->EnableAngularLimit(true);
							joint->SetAngularLimit(angularLimit);
						}

						if (isDrive)
						{
							joint->EnableDrive(true);
							joint->SetDriveVelocity(driveVelocity);
						}

						if (isCycle)
						{
							joint->EnableCycleDrive(true);
							joint->SetCycleDrivePeriod(cyclePeriod, cycleAccel);
						}

						cJointRenderer *jointRenderer = new cJointRenderer();
						jointRenderer->Create(*g_evc->m_sync, joint);
						g_evc->m_sync->AddJoint(joint, jointRenderer);
					}
					break;


					case phys::eJointType::Prismatic:
					{
						const bool isLinearLimit = vt0.second.get<bool>("linear limit");
						const string linearLimitStr1 = vt0.second.get<string>("linear limit config 1"
							, "0 0 0");
						const string linearLimitStr2 = vt0.second.get<string>("linear limit config 2"
							, "0 0 0");
						const Vector3 tconf1 = ParseVector3(linearLimitStr1);
						const Vector3 tconf2 = ParseVector3(linearLimitStr2);

						physx::PxJointLinearLimitPair linearLimit(0,0,physx::PxSpring(0,0));
						linearLimit.lower = tconf1.x;
						linearLimit.upper = tconf1.y;
						linearLimit.stiffness = tconf1.z;
						linearLimit.damping = tconf2.x;
						linearLimit.contactDistance = tconf2.y;
						linearLimit.bounceThreshold = tconf2.z;

						phys::cJoint *joint = new phys::cJoint();
						joint->CreatePrismatic(*g_evc->m_phys
							, sync0->actor, sync0->node->m_transform, pivot0
							, sync1->actor, sync1->node->m_transform, pivot1
							, revoluteAxis);

						if (isLinearLimit)
						{
							joint->EnableLinearLimit(true);
							joint->SetLinearLimit(linearLimit);
						}

						cJointRenderer *jointRenderer = new cJointRenderer();
						jointRenderer->Create(*g_evc->m_sync, joint);
						g_evc->m_sync->AddJoint(joint, jointRenderer);
					}
					break;

					case phys::eJointType::Distance:
					{
						const bool isDistanceLimit = vt0.second.get<bool>("distance limit");
						const float minDist = vt0.second.get<float>("min distance", 0.f);
						const float maxDist = vt0.second.get<float>("max distance", 0.f);

						phys::cJoint *joint = new phys::cJoint();
						joint->CreateDistance(*g_evc->m_phys
							, sync0->actor, sync0->node->m_transform, pivot0
							, sync1->actor, sync1->node->m_transform, pivot1);

						if (isDistanceLimit)
						{
							joint->EnableDistanceLimit(true);
							joint->SetDistanceLimit(minDist, maxDist);
						}

						cJointRenderer *jointRenderer = new cJointRenderer();
						jointRenderer->Create(*g_evc->m_sync, joint);
						g_evc->m_sync->AddJoint(joint, jointRenderer);

					}
					break;

					case phys::eJointType::D6:
					{
						using namespace physx;
						Str128 text;

						int motionVal[6];
						bool driveVal[6];
						struct sDriveParam
						{
							float stiffness;
							float damping;
							float forceLimit;
							bool accel;
						};
						sDriveParam driveConfigs[6];

						for (int i = 0; i < 6; ++i)
						{
							text.Format("d6 motion %d", i);
							motionVal[i] = vt0.second.get<int>(text.c_str(), 0);
						}

						for (int i = 0; i < 6; ++i)
						{
							text.Format("d6 drive %d stiffness", i);
							driveConfigs[i].stiffness = vt0.second.get<float>(text.c_str(), 0.f);
							
							text.Format("d6 drive %d damping", i);
							driveConfigs[i].damping = vt0.second.get<float>(text.c_str(), 0.f);
							
							text.Format("d6 drive %d forcelimit", i);
							driveConfigs[i].forceLimit = vt0.second.get<float>(text.c_str(), 0.f);
							
							text.Format("d6 drive %d accel", i);
							driveConfigs[i].accel = vt0.second.get<bool>(text.c_str(), false);

							driveVal[i] = driveConfigs[i].stiffness != 0;
						}

						physx::PxJointLinearLimit linearLimit(0.f, physx::PxSpring(0.f, 0.f));
						linearLimit.value = vt0.second.get<float>("linear extent", 0.f);
						linearLimit.stiffness = vt0.second.get<float>("linear stiffness", 0.f);
						linearLimit.damping = vt0.second.get<float>("linear damping", 0.f);
						bool isLinearLimit = linearLimit.stiffness != 0.f;

						PxJointAngularLimitPair twistLimit(-PxPi / 2.f, PxPi / 2.f);
						twistLimit.lower = vt0.second.get<float>("angular lower", 0.f);
						twistLimit.upper = vt0.second.get<float>("angular upper", 0.f);
						bool isTwistLimit = twistLimit.lower != 0.f;

						PxJointLimitCone swingLimit(-PxPi / 2.f, PxPi / 2.f);
						swingLimit.yAngle = vt0.second.get<float>("cone yAngle", 0.f);
						swingLimit.zAngle = vt0.second.get<float>("cone zAngle", 0.f);
						bool isSwingLimit = swingLimit.yAngle != 0.f;

						const string linearVelStr = vt0.second.get<string>("linear drive velocity", "0 0 0");
						const Vector3 linearDriveVelocity = ParseVector3(linearVelStr);
						const string angularVelStr = vt0.second.get<string>("angular drive velocity", "0 0 0");
						const Vector3 angularDriveVelocity = ParseVector3(angularVelStr);

						phys::cJoint *joint = new phys::cJoint();
						joint->CreateD6(*g_evc->m_phys
							, sync0->actor, sync0->node->m_transform, pivot0
							, sync1->actor, sync1->node->m_transform, pivot1);

						for (int i = 0; i < 6; ++i)
							joint->SetMotion((PxD6Axis::Enum)i, (PxD6Motion::Enum)motionVal[i]);

						for (int i = 0; i < 6; ++i)
						{
							if (driveVal[i])
							{
								joint->SetD6Drive((PxD6Drive::Enum)i
									, physx::PxD6JointDrive(driveConfigs[i].stiffness
										, driveConfigs[i].damping
										, driveConfigs[i].forceLimit
										, driveConfigs[i].accel));
							}
						}

						if (isLinearLimit)
							joint->SetD6LinearLimit(linearLimit);
						if (isTwistLimit)
							joint->SetD6TwistLimit(twistLimit);
						if (isSwingLimit)
							joint->SetD6SwingLimit(swingLimit);

						joint->SetD6DriveVelocity(linearDriveVelocity, angularDriveVelocity);

						cJointRenderer *jointRenderer = new cJointRenderer();
						jointRenderer->Create(*g_evc->m_sync, joint);
						g_evc->m_sync->AddJoint(joint, jointRenderer);
					}
					break;


					default:
						throw std::exception("joint type error");
					}
				}//~for joint
			}//~joints

		}

	}
	catch (std::exception &e)
	{
		common::Str128 msg;
		msg.Format("Read Error!!, File [ %s ]\n%s"
			, fileName.c_str(), e.what());
		//MessageBoxA(NULL, msg.c_str(), "ERROR", MB_OK);
		return false;
	}

	// return generation sync id
	if (outSyncIds)
		for (auto &kv : syncs)
			outSyncIds->push_back(kv.second->id);

	return nullptr;
}


// create phenotype node from genotype node
cPNode* evc::CreatePhenoTypeNode(graphic::cRenderer &renderer
	, const sGenotypeNode &gnode, const uint generation)
{
	if (gnode.iteration >= 0 && (gnode.generation > generation))
		return nullptr; // iteration node, skip creation

	const Vector3 pos = gnode.transform.pos;
	const Vector3 dim = gnode.transform.scale;
	const Quaternion rot = gnode.transform.rot;;

	int newId = -1;
	switch (gnode.shape)
	{
	case phys::eShapeType::Plane: break;
	case phys::eShapeType::Box:
	{
		const Transform tfm(pos, dim, rot);
		newId = g_evc->m_sync->SpawnBox(renderer, tfm, gnode.density, true);
	}
	break;

	case phys::eShapeType::Sphere:
	{
		const Transform tfm(pos, dim, rot);
		newId = g_evc->m_sync->SpawnSphere(renderer, tfm, dim.x, gnode.density, true);
	}
	break;

	case phys::eShapeType::Capsule:
	{
		const Transform tfm(pos, rot);
		newId = g_evc->m_sync->SpawnCapsule(renderer, tfm, dim.y, dim.x - dim.y
			, gnode.density, true);
	}
	break;

	case phys::eShapeType::Cylinder:
	{
		const Transform tfm(pos, rot);
		newId = g_evc->m_sync->SpawnCylinder(renderer, tfm, dim.y, dim.x, gnode.density, true);
	}
	break;

	case phys::eShapeType::Convex:
	default: 
		assert(0);
		return nullptr;
	}

	if (phys::sSyncInfo *sync = g_evc->m_sync->FindSyncInfo(newId))
	{
		if (gnode.mass != 0.f)
			sync->actor->SetMass(gnode.mass);

		cPNode *pnode = new cPNode();
		pnode->Create(renderer, gnode);
		pnode->m_actor = sync->actor;
		pnode->m_node = sync->node;
		return pnode;
	}

	return nullptr;
}


// create joint from genotype link 
phys::cJoint* evc::CreatePhenoTypeJoint(const sGenotypeLink &glink
	, cPNode *pnode0, cPNode *pnode1, const uint generation)
{
	using namespace physx;

	const Vector3 pivot0 = glink.pivots[0].dir * glink.nodeLocal0.rot * glink.pivots[0].len
		+ glink.nodeLocal0.pos;
	const Vector3 pivot1 = glink.pivots[1].dir * glink.nodeLocal1.rot * glink.pivots[1].len
		+ glink.nodeLocal1.pos;
	const Vector3 revoluteAxis = glink.revoluteAxis;

	phys::cJoint *joint = nullptr;
	switch (glink.type)
	{
	case phys::eJointType::Fixed:
	{
		joint = new phys::cJoint();
		joint->CreateFixed(*g_evc->m_phys
			, pnode0->m_actor, pnode0->m_node->m_transform, pivot0
			, pnode1->m_actor, pnode1->m_node->m_transform, pivot1);

		cJointRenderer *jointRenderer = new cJointRenderer();
		jointRenderer->Create(*g_evc->m_sync, joint);
		g_evc->m_sync->AddJoint(joint, jointRenderer);
	}
	break;

	case phys::eJointType::Spherical:
	{
		joint = new phys::cJoint();
		joint->CreateSpherical(*g_evc->m_phys
			, pnode0->m_actor, pnode0->m_node->m_transform, pivot0
			, pnode1->m_actor, pnode1->m_node->m_transform, pivot1);

		if (glink.limit.cone.isLimit)
		{
			joint->EnableConeLimit(true);
			joint->SetConeLimit(
				PxJointLimitCone(glink.limit.cone.yAngle, glink.limit.cone.zAngle, 0.01f));
		}

		cJointRenderer *jointRenderer = new cJointRenderer();
		jointRenderer->Create(*g_evc->m_sync, joint);
		g_evc->m_sync->AddJoint(joint, jointRenderer);
	}
	break;

	case phys::eJointType::Revolute:
	{
		joint = new phys::cJoint();
		joint->CreateRevolute(*g_evc->m_phys
			, pnode0->m_actor, pnode0->m_node->m_transform, pivot0
			, pnode1->m_actor, pnode1->m_node->m_transform, pivot1
			, revoluteAxis);

		if (glink.limit.angular.isLimit)
		{
			joint->EnableAngularLimit(true);
			joint->SetAngularLimit(
				PxJointAngularLimitPair(glink.limit.angular.lower, glink.limit.angular.upper, 0.01f));
		}

		if (glink.drive.isDrive)
		{
			joint->EnableDrive(true);
			joint->SetDriveVelocity(glink.drive.velocity);
		}

		if (glink.drive.isCycle)
		{
			joint->EnableCycleDrive(true);
			joint->SetCycleDrivePeriod(glink.drive.period, glink.drive.driveAccel);
		}

		cJointRenderer *jointRenderer = new cJointRenderer();
		jointRenderer->Create(*g_evc->m_sync, joint);
		g_evc->m_sync->AddJoint(joint, jointRenderer);
	}
	break;


	case phys::eJointType::Prismatic:
	{
		joint = new phys::cJoint();
		joint->CreatePrismatic(*g_evc->m_phys
			, pnode0->m_actor, pnode0->m_node->m_transform, pivot0
			, pnode1->m_actor, pnode1->m_node->m_transform, pivot1
			, revoluteAxis);

		if (glink.limit.linear.isLimit)
		{
			joint->EnableLinearLimit(true);
			
			physx::PxJointLinearLimitPair linearLimit(0, 0, physx::PxSpring(0, 0));
			linearLimit.lower = glink.limit.linear.lower;
			linearLimit.upper = glink.limit.linear.upper;
			linearLimit.stiffness = glink.limit.linear.stiffness;
			linearLimit.damping = glink.limit.linear.damping;
			linearLimit.contactDistance = glink.limit.linear.contactDistance;
			linearLimit.bounceThreshold = glink.limit.linear.bounceThreshold;
			joint->SetLinearLimit(linearLimit);
		}

		cJointRenderer *jointRenderer = new cJointRenderer();
		jointRenderer->Create(*g_evc->m_sync, joint);
		g_evc->m_sync->AddJoint(joint, jointRenderer);
	}
	break;

	case phys::eJointType::Distance:
	{
		joint = new phys::cJoint();
		joint->CreateDistance(*g_evc->m_phys
			, pnode0->m_actor, pnode0->m_node->m_transform, pivot0
			, pnode1->m_actor, pnode1->m_node->m_transform, pivot1);

		if (glink.limit.distance.isLimit)
		{
			joint->EnableDistanceLimit(true);
			joint->SetDistanceLimit(glink.limit.distance.minDistance
				, glink.limit.distance.maxDistance);
		}

		cJointRenderer *jointRenderer = new cJointRenderer();
		jointRenderer->Create(*g_evc->m_sync, joint);
		g_evc->m_sync->AddJoint(joint, jointRenderer);
	}
	break;

	case phys::eJointType::D6:
	{
		joint = new phys::cJoint();
		joint->CreateD6(*g_evc->m_phys
			, pnode0->m_actor, pnode0->m_node->m_transform, pivot0
			, pnode1->m_actor, pnode1->m_node->m_transform, pivot1);

		for (int i = 0; i < 6; ++i)
			joint->SetMotion((PxD6Axis::Enum)i, (PxD6Motion::Enum)glink.limit.d6.motion[i]);

		for (int i = 0; i < 6; ++i)
		{
			if (glink.limit.d6.drive[i].stiffness != 0.f)
			{
				joint->SetD6Drive((PxD6Drive::Enum)i
					, physx::PxD6JointDrive(glink.limit.d6.drive[i].stiffness
						, glink.limit.d6.drive[i].damping
						, glink.limit.d6.drive[i].forceLimit
						, glink.limit.d6.drive[i].accel));
			}
		}

		if (glink.limit.d6.linear.isLimit)
		{
			physx::PxJointLinearLimit linearLimit(0.f, physx::PxSpring(0.f, 0.f));
			linearLimit.value = glink.limit.d6.linear.value;
			linearLimit.stiffness = glink.limit.d6.linear.stiffness;
			linearLimit.damping = glink.limit.d6.linear.damping;
			joint->SetD6LinearLimit(linearLimit);
		}
		if (glink.limit.d6.twist.isLimit)
		{
			PxJointAngularLimitPair twistLimit(-PxPi / 2.f, PxPi / 2.f);
			twistLimit.lower = glink.limit.d6.twist.lower;
			twistLimit.upper = glink.limit.d6.twist.upper;
			joint->SetD6TwistLimit(twistLimit);
		}
		if (glink.limit.d6.swing.isLimit)
		{
			PxJointLimitCone swingLimit(-PxPi / 2.f, PxPi / 2.f);
			swingLimit.yAngle = glink.limit.d6.swing.yAngle;
			swingLimit.zAngle = glink.limit.d6.swing.zAngle;
			joint->SetD6SwingLimit(swingLimit);
		}

		joint->SetD6DriveVelocity(*(Vector3*)glink.limit.d6.linearVelocity
			, *(Vector3*)glink.limit.d6.angularVelocity);

		cJointRenderer *jointRenderer = new cJointRenderer();
		jointRenderer->Create(*g_evc->m_sync, joint);
		g_evc->m_sync->AddJoint(joint, jointRenderer);
	}
	break;

	default:
		assert(0);
		return nullptr;
	}

	return joint;
}



// parse string to Vector4
// string format : x y z w
Vector4 evc::ParseVector4(const string &str)
{
	vector<string> toks;
	common::tokenizer(str, " ", "", toks);
	if (toks.size() >= 4)
	{
		return Vector4((float)atof(toks[0].c_str())
			, (float)atof(toks[1].c_str())
			, (float)atof(toks[2].c_str())
			, (float)atof(toks[3].c_str()));
	}
	return Vector3::Zeroes;
}


// parse string to Vector3
// string format : x y z
Vector3 evc::ParseVector3(const string &str)
{
	vector<string> toks;
	common::tokenizer(str, " ", "", toks);
	if (toks.size() >= 3)
	{
		return Vector3((float)atof(toks[0].c_str())
			, (float)atof(toks[1].c_str())
			, (float)atof(toks[2].c_str()));
	}
	return Vector3::Zeroes;
}


// parse string to Vector2
// string format : x y
Vector2 evc::ParseVector2(const string &str)
{
	vector<string> toks;
	common::tokenizer(str, " ", "", toks);
	if (toks.size() >= 2)
	{
		return Vector2((float)atof(toks[0].c_str())
			, (float)atof(toks[1].c_str()));
	}
	return Vector2(0, 0);
}


string evc::Vector2ToStr(const Vector2 &vec)
{
	stringstream ss;
	ss << "vec2(" << vec.x << "," << vec.y << ")";
	return ss.str();
}


string evc::Vector3ToStr(const Vector3 &vec)
{
	return Vector3ToStr("vec3", vec);
}


string evc::Vector3ToStr(const string &name, const Vector3 &vec)
{
	stringstream ss;
	ss << name << "(" << vec.x << "," << vec.y << "," << vec.z << ")";
	return ss.str();
}


string evc::Vector4ToStr(const Vector4 &vec)
{
	stringstream ss;
	ss << "vec4(" << vec.x << "," << vec.y << "," << vec.z << "," << vec.w << ")";
	return ss.str();
}


string evc::QuaternionToStr(const Quaternion &q)
{
	stringstream ss;
	ss << "quat(" << q.x << "," << q.y << "," << q.z << "," << q.w << ")";
	return ss.str();
}


string evc::TransformToStr(const Transform &tfm)
{
	stringstream ss;
	ss << "transform(" << Vector3ToStr(tfm.pos)
		<< "," << QuaternionToStr(tfm.rot) << ")";
	return ss.str();
}


string evc::ConeLimitToStr(const sConeLimit &limit)
{
	stringstream ss;
	ss << "conelimit(" << limit.yAngle << "," << limit.zAngle << ")";
	return ss.str();
}


string evc::AngularLimitToStr(const sAngularLimit &limit)
{
	stringstream ss;
	ss << "angularlimit(" << limit.lower << "," << limit.upper << ")";
	return ss.str();
}


string evc::LinearLimitToStr(const sLinearLimit &limit)
{
	stringstream ss;
	ss << "linearlimit(" << limit.lower << "," << limit.upper 
		<< "," << limit.stiffness << "," << limit.damping << ")";
	return ss.str();
}


string evc::DistanceLimitToStr(const sDistanceLimit &limit)
{
	stringstream ss;
	ss << "distancelimit(" << limit.minDistance << "," << limit.maxDistance << ")";
	return ss.str();
}


string evc::DriveInfoToStr(const sDriveInfo &drive)
{
	stringstream ss;
	ss << "drive("
		<< "velocity(" << drive.velocity << ")"
		<< ",period(" << drive.period << ")"
		<< ")";
	return ss.str();
}


// put rigid actor property to parent ptree
bool evc::Put_RigidActor(boost::property_tree::ptree &parent, phys::cRigidActor *actor)
{
	using boost::property_tree::ptree;
	Vector3 v;
	Quaternion q;
	common::Str128 text;

	ptree shape;

	shape.put("name", "blank");
	shape.put<int>("id", actor->m_id);
	shape.put<string>("type", phys::eRigidType::ToString(actor->m_type));
	shape.put<string>("shape", phys::eShapeType::ToString(actor->m_shape));

	// find local transform from joint
	Transform tfm;
	for (auto &j : actor->m_joints)
	{
		if (j->m_actor0 == actor)
		{
			tfm = j->m_actorLocal0;
			break;
		}
		if (j->m_actor1 == actor)
		{
			tfm = j->m_actorLocal1;
			break;
		}
	}
	if (actor->m_joints.empty()) // not found transform?
	{
		if (phys::sSyncInfo *sync = g_evc->m_sync->FindSyncInfo(actor))
			tfm = sync->node->m_transform;
	}

	v = tfm.pos;
	text.Format("%f %f %f", v.x, v.y, v.z);
	shape.put("pos", text.c_str());

	v = tfm.scale;
	text.Format("%f %f %f", v.x, v.y, v.z);
	shape.put("dim", text.c_str());

	q = tfm.rot;
	text.Format("%f %f %f %f", q.x, q.y, q.z, q.w);
	shape.put("rot", text.c_str());

	shape.put<float>("mass", actor->GetMass());
	shape.put<float>("angular damping", actor->GetAngularDamping());
	shape.put<float>("linear damping", actor->GetLinearDamping());
	shape.put<bool>("kinematic", actor->IsKinematic());

	parent.add_child("shape", shape);

	return true;
}


// put joint property to parent ptree
bool evc::Put_Joint(boost::property_tree::ptree &parent, phys::cJoint *joint)
{
	using boost::property_tree::ptree;
	Vector3 v;
	Quaternion q;
	common::Str128 text;

	ptree j;

	j.put("type", phys::eJointType::ToString(joint->m_type));
	j.put("shape id0", joint->m_actor0->m_id);
	j.put("shape id1", joint->m_actor1->m_id);

	v = joint->m_origPos;
	text.Format("%f %f %f", v.x, v.y, v.z);
	j.put("jointpos", text.c_str());

	q = joint->m_rotRevolute;
	text.Format("%f %f %f %f", q.x, q.y, q.z, q.w);
	j.put("revolute rot", text.c_str());

	v = joint->m_pivots[0].dir;
	text.Format("%f %f %f", v.x, v.y, v.z);
	j.put("pivot dir0", text.c_str());
	j.put("pivot len0", joint->m_pivots[0].len);

	v = joint->m_pivots[1].dir;
	text.Format("%f %f %f", v.x, v.y, v.z);
	j.put("pivot dir1", text.c_str());
	j.put("pivot len1", joint->m_pivots[1].len);

	switch (joint->m_type)
	{
	case phys::eJointType::Fixed: break;
	case phys::eJointType::Spherical:
	{
		j.put<bool>("cone limit", joint->IsConeLimit());
		physx::PxJointLimitCone coneLimit = joint->GetConeLimit();
		text.Format("%f %f %f", coneLimit.yAngle, coneLimit.zAngle, 0.01f);
		j.put<string>("cone limit config", text.c_str());
	}
	break;
	case phys::eJointType::Revolute:
	{
		j.put<bool>("drive", joint->IsDrive());
		j.put("drive velocity", joint->m_maxDriveVelocity);

		j.put<bool>("cycle", joint->m_isCycleDrive);
		j.put("cycle period", joint->m_cyclePeriod);
		j.put("cycle accel", joint->m_cycleDriveAccel);

		j.put<bool>("cone limit", joint->IsConeLimit());
		physx::PxJointLimitCone coneLimit = joint->GetConeLimit();
		text.Format("%f %f %f", coneLimit.yAngle, coneLimit.zAngle, 0.01f);
		j.put<string>("cone limit config", text.c_str());

		j.put<bool>("angular limit", joint->IsAngularLimit());
		physx::PxJointAngularLimitPair angularLimit = joint->GetAngularLimit();
		text.Format("%f %f %f", angularLimit.lower, angularLimit.upper, 0.01f);
		j.put<string>("angular limit config", text.c_str());
	}
	break;

	case phys::eJointType::Prismatic:
	{
		j.put<bool>("linear limit", joint->IsLinearLimit());
		physx::PxJointLinearLimitPair linearLimit = joint->GetLinearLimit();
		text.Format("%f %f %f", linearLimit.lower, linearLimit.upper
			, linearLimit.stiffness);
		j.put<string>("linear limit config 1", text.c_str());
		text.Format("%f %f %f", linearLimit.damping, linearLimit.contactDistance
			, linearLimit.bounceThreshold);
		j.put<string>("linear limit config 2", text.c_str());
	}
	break;

	case phys::eJointType::Distance:
	{
		j.put<bool>("distance limit", joint->IsLinearLimit());
		const Vector2 dist = joint->GetDistanceLimit();
		j.put<float>("min distance", dist.x);
		j.put<float>("max distance", dist.y);
	}
	break;

	case phys::eJointType::D6:
	{
		for (int i = 0; i < 6; ++i)
		{
			const physx::PxD6Motion::Enum motion = joint->GetMotion((physx::PxD6Axis::Enum)i);
			text.Format("d6 motion %d", i);
			j.put<int>(text.c_str(), (int)motion);
		}

		for (int i = 0; i < 6; ++i)
		{
			const physx::PxD6JointDrive drive = joint->GetD6Drive((physx::PxD6Drive::Enum)i);
			text.Format("d6 drive %d stiffness", i);
			j.put<float>(text.c_str(), drive.stiffness);
			text.Format("d6 drive %d damping", i);
			j.put<float>(text.c_str(), drive.damping);
			text.Format("d6 drive %d forcelimit", i);
			j.put<float>(text.c_str(), drive.forceLimit);
			const bool accel = drive.flags.isSet(physx::PxD6JointDriveFlag::eACCELERATION);
			text.Format("d6 drive %d accel", i);
			j.put<bool>(text.c_str(), accel);
		}

		const physx::PxJointLinearLimit linearLimit = joint->GetD6LinearLimit();
		j.put<float>("linear extent", linearLimit.value);
		j.put<float>("linear stiffness", linearLimit.stiffness);
		j.put<float>("linear damping", linearLimit.damping);

		const physx::PxJointAngularLimitPair angularLimit = joint->GetD6TwistLimit();
		j.put<float>("angular lower", angularLimit.lower);
		j.put<float>("angular upper", angularLimit.upper);

		const physx::PxJointLimitCone coneLimit = joint->GetD6SwingLimit();
		j.put<float>("cone yAngle", coneLimit.yAngle);
		j.put<float>("cone zAngle", coneLimit.zAngle);

		const std::pair<Vector3, Vector3> driveVelocity =
			joint->GetD6DriveVelocity();

		const Vector3 linearVel = std::get<0>(driveVelocity);
		const Vector3 angularVel = std::get<1>(driveVelocity);
		text.Format("%f %f %f", linearVel.x, linearVel.y, linearVel.z);
		j.put<string>("linear drive velocity", text.c_str());
		text.Format("%f %f %f", angularVel.x, angularVel.y, angularVel.z);
		j.put<string>("angular drive velocity", text.c_str());
	}
	break;
	}

	parent.add_child("joint", j);

	return true;
}


// put gnode properties
bool evc::Put_GNode(boost::property_tree::ptree &parent, evc::cGNode *gnode)
{
	using boost::property_tree::ptree;
	Vector3 v;
	Quaternion q;
	common::Str128 text;

	ptree shape;

	shape.put("name", gnode->m_name.c_str());
	shape.put<int>("id", gnode->m_id);
	shape.put<string>("type", "Dynamic");
	shape.put<string>("shape", phys::eShapeType::ToString(gnode->m_shape));

	// find local transform from joint
	Transform tfm = gnode->m_transform;
	switch (gnode->m_shape)
	{
	case phys::eShapeType::Box: break;
	case phys::eShapeType::Sphere: break;
	case phys::eShapeType::Capsule:
	{
		const Vector2 dim = gnode->GetCapsuleDimension();
		tfm.scale = Vector3(dim.x + dim.y, dim.x, dim.x);
	}
	break;
	case phys::eShapeType::Cylinder:
	{
		const Vector2 dim = gnode->GetCylinderDimension();
		tfm.scale = Vector3(dim.y, dim.x, dim.x);
	}
	break;
	default: assert(0); break;
	}

	v = tfm.pos;
	text.Format("%f %f %f", v.x, v.y, v.z);
	shape.put("pos", text.c_str());

	v = tfm.scale;
	text.Format("%f %f %f", v.x, v.y, v.z);
	shape.put("dim", text.c_str());

	q = tfm.rot;
	text.Format("%f %f %f %f", q.x, q.y, q.z, q.w);
	shape.put("rot", text.c_str());

	shape.put<float>("mass", 0.f);
	shape.put<float>("density", gnode->m_density);
	shape.put<float>("angular damping", 0.5f);
	shape.put<float>("linear damping", 0.5f);
	shape.put<bool>("kinematic", false);
	shape.put<int>("iteration", gnode->m_cloneId);
	shape.put<unsigned int>("max generation", gnode->m_maxGeneration);

	parent.add_child("shape", shape);

	return true;
}


// put genotype link property to parent ptree
bool evc::Put_GLink(boost::property_tree::ptree &parent, evc::cGLink *glink)
{
	using boost::property_tree::ptree;
	Vector3 v;
	Quaternion q;
	common::Str128 text;

	ptree j;

	j.put("type", phys::eJointType::ToString(glink->m_type));
	j.put("shape id0", glink->m_gnode0->m_id);
	j.put("shape id1", glink->m_gnode1->m_id);

	v = glink->m_origPos;
	text.Format("%f %f %f", v.x, v.y, v.z);
	j.put("jointpos", text.c_str());

	q = glink->m_rotRevolute;
	text.Format("%f %f %f %f", q.x, q.y, q.z, q.w);
	j.put("revolute rot", text.c_str());

	v = glink->m_pivots[0].dir;
	text.Format("%f %f %f", v.x, v.y, v.z);
	j.put("pivot dir0", text.c_str());
	j.put("pivot len0", glink->m_pivots[0].len);

	v = glink->m_pivots[1].dir;
	text.Format("%f %f %f", v.x, v.y, v.z);
	j.put("pivot dir1", text.c_str());
	j.put("pivot len1", glink->m_pivots[1].len);

	switch (glink->m_type)
	{
	case phys::eJointType::Fixed: break;
	case phys::eJointType::Spherical:
	{
		j.put<bool>("cone limit", glink->m_limit.cone.isLimit);
		const sConeLimit coneLimit = glink->m_limit.cone;
		text.Format("%f %f %f", coneLimit.yAngle, coneLimit.zAngle, 0.01f);
		j.put<string>("cone limit config", text.c_str());
	}
	break;
	case phys::eJointType::Revolute:
	{
		j.put<bool>("drive", glink->m_drive.isDrive);
		j.put("drive velocity", glink->m_drive.velocity);

		j.put<bool>("cycle", glink->m_drive.isCycle);
		j.put("cycle period", glink->m_drive.period);
		j.put("cycle accel", glink->m_drive.driveAccel);

		j.put<bool>("cone limit", glink->m_limit.cone.isLimit);
		const sConeLimit coneLimit = glink->m_limit.cone;
		text.Format("%f %f %f", coneLimit.yAngle, coneLimit.zAngle, 0.01f);
		j.put<string>("cone limit config", text.c_str());

		j.put<bool>("angular limit", glink->m_limit.angular.isLimit);
		const sAngularLimit angularLimit = glink->m_limit.angular;
		text.Format("%f %f %f", angularLimit.lower, angularLimit.upper, 0.01f);
		j.put<string>("angular limit config", text.c_str());
	}
	break;

	case phys::eJointType::Prismatic:
	{
		j.put<bool>("linear limit", glink->m_limit.linear.isLimit);
		j.put<bool>("spring limit", glink->m_limit.linear.isSpring);
		const sLinearLimit linearLimit = glink->m_limit.linear;
		text.Format("%f %f %f", linearLimit.lower, linearLimit.upper
			, linearLimit.stiffness);
		j.put<string>("linear limit config 1", text.c_str());
		text.Format("%f %f %f", linearLimit.damping, linearLimit.contactDistance
			, linearLimit.bounceThreshold);
		j.put<string>("linear limit config 2", text.c_str());
	}
	break;

	case phys::eJointType::Distance:
	{
		j.put<bool>("distance limit", glink->m_limit.distance.isLimit);
		const sDistanceLimit dist = glink->m_limit.distance;
		j.put<float>("min distance", dist.minDistance);
		j.put<float>("max distance", dist.maxDistance);
	}
	break;

	case phys::eJointType::D6:
	{
		for (int i = 0; i < 6; ++i)
		{
	//		const physx::PxD6Motion::Enum motion = joint->GetMotion((physx::PxD6Axis::Enum)i);
	//		text.Format("d6 motion %d", i);
	//		j.put<int>(text.c_str(), (int)motion);
		}

		for (int i = 0; i < 6; ++i)
		{
	//		const physx::PxD6JointDrive drive = joint->GetD6Drive((physx::PxD6Drive::Enum)i);
	//		text.Format("d6 drive %d stiffness", i);
	//		j.put<float>(text.c_str(), drive.stiffness);
	//		text.Format("d6 drive %d damping", i);
	//		j.put<float>(text.c_str(), drive.damping);
	//		text.Format("d6 drive %d forcelimit", i);
	//		j.put<float>(text.c_str(), drive.forceLimit);
	//		const bool accel = drive.flags.isSet(physx::PxD6JointDriveFlag::eACCELERATION);
	//		text.Format("d6 drive %d accel", i);
	//		j.put<bool>(text.c_str(), accel);
		}

	//	const physx::PxJointLinearLimit linearLimit = joint->GetD6LinearLimit();
	//	j.put<float>("linear extent", linearLimit.value);
	//	j.put<float>("linear stiffness", linearLimit.stiffness);
	//	j.put<float>("linear damping", linearLimit.damping);

	//	const physx::PxJointAngularLimitPair angularLimit = joint->GetD6TwistLimit();
	//	j.put<float>("angular lower", angularLimit.lower);
	//	j.put<float>("angular upper", angularLimit.upper);

	//	const physx::PxJointLimitCone coneLimit = joint->GetD6SwingLimit();
	//	j.put<float>("cone yAngle", coneLimit.yAngle);
	//	j.put<float>("cone zAngle", coneLimit.zAngle);

	//	const std::pair<Vector3, Vector3> driveVelocity =
	//		joint->GetD6DriveVelocity();

	//	const Vector3 linearVel = std::get<0>(driveVelocity);
	//	const Vector3 angularVel = std::get<1>(driveVelocity);
	//	text.Format("%f %f %f", linearVel.x, linearVel.y, linearVel.z);
	//	j.put<string>("linear drive velocity", text.c_str());
	//	text.Format("%f %f %f", angularVel.x, angularVel.y, angularVel.z);
	//	j.put<string>("angular drive velocity", text.c_str());
	}
	break;
	}

	parent.add_child("joint", j);

	return true;
}


// write genotype file from Genotype node
bool evc::WriteGenoTypeFileFrom_Node(const StrPath &fileName
	, cGNode *gnode)
{
	// collect gnode, link
	set<cGNode*> gnodes;
	set<cGLink*> glinks;
	queue<cGNode*> q;
	q.push(gnode);
	while (!q.empty())
	{
		cGNode *node = q.front();
		q.pop();
		if (!node)
			continue;
		if (gnodes.end() != std::find(gnodes.begin(), gnodes.end(), node))
			continue; // already exist

		gnodes.insert(node);
		for (auto *link : node->m_links)
		{
			q.push(link->m_gnode0);
			q.push(link->m_gnode1);
			glinks.insert(link);
		}
	}

	// make ptree
	try
	{
		using boost::property_tree::ptree;
		ptree props;
		ptree creature;
		common::Str128 text;
		Vector3 v;
		Quaternion q;

		creature.put("version", "1.01");
		creature.put("name", "creature genotype");

		// make shape
		ptree shapes;
		for (auto &p : gnodes)
			Put_GNode(shapes, p);

		// make joint
		ptree jts;
		for (auto &p : glinks)
		{
			// check exist gnode?
			auto it0 = std::find(gnodes.begin(), gnodes.end(), p->m_gnode0);
			auto it1 = std::find(gnodes.begin(), gnodes.end(), p->m_gnode1);
			if ((gnodes.end() == it0) || (gnodes.end() == it1))
				continue; // ignore link, if not exist gnode
			Put_GLink(jts, p);
		}

		creature.add_child("shapes", shapes);
		creature.add_child("joints", jts);
		props.add_child("creature", creature);
		boost::property_tree::write_json(fileName.c_str(), props);
	}
	catch (std::exception &e)
	{
		common::Str128 msg;
		msg.Format("Write Error2!!, File [ %s ]\n%s"
			, fileName.c_str(), e.what());
		//MessageBoxA(NULL, msg.c_str(), "ERROR", MB_OK);
		return false;
	}

	return true;
}


// write genotype file
bool evc::WriteGenoTypeFileFrom_Node(const StrPath &fileName
	, const vector<cGNode*> &gnodes)
{
	// collect gnode, link
	set<cGLink*> glinks;
	for (auto &p : gnodes)
	{
		for (auto *l : p->m_links)
			glinks.insert(l);
	}

	// make ptree
	try
	{
		using boost::property_tree::ptree;
		ptree props;
		ptree creature;
		common::Str128 text;
		Vector3 v;
		Quaternion q;

		creature.put("version", "1.01");
		creature.put("name", "creature genotype");

		// make shape
		ptree shapes;
		for (auto &p : gnodes)
			Put_GNode(shapes, p);

		// make joint
		ptree jts;
		for (auto &p : glinks)
		{
			// check exist gnode?
			auto it0 = std::find(gnodes.begin(), gnodes.end(), p->m_gnode0);
			auto it1 = std::find(gnodes.begin(), gnodes.end(), p->m_gnode1);
			if ((gnodes.end() == it0) || (gnodes.end() == it1))
				continue; // ignore link, if not exist gnode
			Put_GLink(jts, p);
		}

		creature.add_child("shapes", shapes);
		creature.add_child("joints", jts);
		props.add_child("creature", creature);
		boost::property_tree::write_json(fileName.c_str(), props);
	}
	catch (std::exception &e)
	{
		common::Str128 msg;
		msg.Format("Write Error2!!, File [ %s ]\n%s"
			, fileName.c_str(), e.what());
		//MessageBoxA(NULL, msg.c_str(), "ERROR", MB_OK);
		return false;
	}

	return true;
}


// read genotype file
// return cGNode, cGLink ptr array
bool evc::ReadGenoTypeFile(const StrPath &fileName
	, OUT vector<sGenotypeNode*> &outNode
	, OUT vector<sGenotypeLink*> &outLink
	, OUT map<int,sGenotypeNode*> &outMap)
{
	using boost::property_tree::ptree;

	map<int, sGenotypeNode*> gnodes; //key: gnode id

	try
	{
		ptree props;
		boost::property_tree::read_json(fileName.c_str(), props);

		// parse creature
		ptree::assoc_iterator itor0 = props.find("creature");
		if (props.not_found() != itor0)
		{
			if (props.not_found() != itor0->second.find("shapes"))
			{
				ptree &child_field0 = itor0->second.get_child("shapes");
				for (ptree::value_type &vt0 : child_field0)
				{
					const string name = vt0.second.get<string>("name", "blank");
					const int id = vt0.second.get<int>("id");
					const phys::eRigidType::Enum type = phys::eRigidType::FromString(
						vt0.second.get<string>("type", "Dynamic"));
					const phys::eShapeType::Enum shape = phys::eShapeType::FromString(
						vt0.second.get<string>("shape", "Box"));
					const string posStr = vt0.second.get<string>("pos", "0 0 0");
					const Vector3 pos = ParseVector3(posStr);
					const string dimStr = vt0.second.get<string>("dim", "1 1 1");
					const Vector3 dim = ParseVector3(dimStr);
					const string rotStr = vt0.second.get<string>("rot", "0 0 0 1");
					const Vector4 rot = ParseVector4(rotStr);
					const Quaternion q(rot.x, rot.y, rot.z, rot.w);
					const float mass = vt0.second.get<float>("mass", 0.f);
					const float density = vt0.second.get<float>("density", 1.f);
					const float angularDamping = vt0.second.get<float>("angular damping", 1.f);
					const float linearDamping = vt0.second.get<float>("linear damping", 1.f);
					const bool kinematic = vt0.second.get<bool>("kinematic", true);
					const int iteration = vt0.second.get<int>("iteration", -1);
					const uint maxGeneration = vt0.second.get<unsigned int>("max generation", 0);

					sGenotypeNode *gnode = new sGenotypeNode;
					gnode->id = id;
					gnode->name = name;
					gnode->shape = shape;
					gnode->transform = Transform(pos, dim, q);
					gnode->density = density;
					gnode->color = graphic::cColor::WHITE;
					gnode->mass = mass;
					gnode->linearDamping = linearDamping;
					gnode->angularDamping = angularDamping;
					gnode->iteration = iteration;
					gnode->maxGeneration = maxGeneration;
					gnode->generation = (iteration < 0)? 0 : 1;
					gnode->clonable = true;
					outNode.push_back(gnode);

					gnodes[id] = gnode;

				}//~for shapes
			}//~shapes

			if (props.not_found() != itor0->second.find("joints"))
			{
				ptree &child_field0 = itor0->second.get_child("joints");
				for (ptree::value_type &vt0 : child_field0)
				{
					const phys::eJointType::Enum type = phys::eJointType::FromString(
						vt0.second.get<string>("type", "Fixed"));
					const int actorId0 = vt0.second.get<int>("shape id0");
					const int actorId1 = vt0.second.get<int>("shape id1");
					const string revoluteRotStr = vt0.second.get<string>("revolute rot", "0 0 0 1");
					const Vector4 rot = ParseVector4(revoluteRotStr);
					const Quaternion revoluteQ(rot.x, rot.y, rot.z, rot.w);
					const string pivotDirStr0 = vt0.second.get<string>("pivot dir0", "1 0 0");
					const string pivotDirStr1 = vt0.second.get<string>("pivot dir1", "1 0 0");
					const Vector3 pivotDir0 = ParseVector3(pivotDirStr0);
					const Vector3 pivotDir1 = ParseVector3(pivotDirStr1);
					const float pivotLen0 = vt0.second.get<float>("pivot len0");
					const float pivotLen1 = vt0.second.get<float>("pivot len1");

					auto it0 = gnodes.find(actorId0);
					auto it1 = gnodes.find(actorId1);
					if ((gnodes.end() == it0) || (gnodes.end() == it1))
						continue; // error but continue

					sGenotypeNode *gnode0 = it0->second;
					sGenotypeNode *gnode1 = it1->second;

					// GetPivotWorldTransform
					const Transform tfm0 = gnode0->transform;
					const Transform tfm1 = gnode1->transform;
					const Vector3 localPos0 = pivotDir0 * tfm0.rot * pivotLen0;
					const Vector3 pivot0 = tfm0.pos + localPos0;
					const Vector3 localPos1 = pivotDir1 * tfm1.rot * pivotLen1;
					const Vector3 pivot1 = tfm1.pos + localPos1;
					const Vector3 revoluteAxis = Vector3(1, 0, 0) * revoluteQ;

					sGenotypeLink *glink = new sGenotypeLink;
					glink->type = type;
					glink->parent = gnode0;
					glink->child = gnode1;
					glink->origPos = (pivot0 + pivot1) / 2.f;
					glink->revoluteAxis = revoluteAxis;
					glink->rotRevolute = revoluteQ;
					glink->drive.isCycle = false;
					glink->nodeLocal0 = gnode0->transform;
					glink->nodeLocal1 = gnode1->transform;
					glink->pivots[0].dir = (pivot0 - tfm0.pos).Normal() * tfm0.rot.Inverse();
					glink->pivots[0].len = (pivot0 - tfm0.pos).Length();
					glink->pivots[1].dir = (pivot1 - tfm1.pos).Normal() * tfm1.rot.Inverse();
					glink->pivots[1].len = (pivot1 - tfm1.pos).Length();

					switch (type)
					{
					case phys::eJointType::Fixed: break;

					case phys::eJointType::Spherical:
					{
						const bool isConeLimit = vt0.second.get<bool>("cone limit", false);
						const string coneLimitStr = vt0.second.get<string>("cone limit config", "0 0 0.01");
						const Vector3 tconf0 = ParseVector3(coneLimitStr);
						glink->limit.cone.isLimit = isConeLimit;
						glink->limit.cone.yAngle = tconf0.x;
						glink->limit.cone.zAngle = tconf0.y;
					}
					break;

					case phys::eJointType::Revolute:
					{
						const bool isDrive = vt0.second.get<bool>("drive");
						const float driveVelocity = vt0.second.get<float>("drive velocity");
						const bool isCycle = vt0.second.get<bool>("cycle", false);
						const float cyclePeriod = vt0.second.get<float>("cycle period", 0.f);
						const float cycleAccel = vt0.second.get<float>("cycle accel", 0.f);

						const bool isAngularLimit = vt0.second.get<bool>("angular limit", false);
						const string angularLimitStr = vt0.second.get<string>("angular limit config", "0 0 0.01");
						const Vector3 tconf1 = ParseVector3(angularLimitStr);

						glink->drive.isDrive = isDrive;
						glink->drive.velocity = driveVelocity;
						glink->drive.isCycle = isCycle;
						glink->drive.period = cyclePeriod;
						glink->drive.driveAccel = cycleAccel;
						glink->drive.maxVelocity = driveVelocity;

						glink->limit.angular.isLimit = isAngularLimit;
						glink->limit.angular.lower = tconf1.x;
						glink->limit.angular.upper = tconf1.y;
					}
					break;


					case phys::eJointType::Prismatic:
					{
						const bool isLinearLimit = vt0.second.get<bool>("linear limit");
						const bool isSpringLimit = vt0.second.get<bool>("spring limit");
						const string linearLimitStr1 = vt0.second.get<string>("linear limit config 1"
							, "0 0 0");
						const string linearLimitStr2 = vt0.second.get<string>("linear limit config 2"
							, "0 0 0");
						const Vector3 tconf1 = ParseVector3(linearLimitStr1);
						const Vector3 tconf2 = ParseVector3(linearLimitStr2);

						glink->limit.linear.isLimit = isLinearLimit;
						glink->limit.linear.isSpring = isSpringLimit;
						glink->limit.linear.lower = tconf1.x;
						glink->limit.linear.upper = tconf1.y;
						glink->limit.linear.stiffness = tconf1.z;
						glink->limit.linear.damping = tconf2.x;
						glink->limit.linear.contactDistance = tconf2.y;
						glink->limit.linear.bounceThreshold = tconf2.z;
					}
					break;

					case phys::eJointType::Distance:
					{
						const bool isDistanceLimit = vt0.second.get<bool>("distance limit");
						const float minDist = vt0.second.get<float>("min distance", 0.f);
						const float maxDist = vt0.second.get<float>("max distance", 0.f);

						glink->limit.distance.isLimit = isDistanceLimit;
						glink->limit.distance.minDistance = minDist;
						glink->limit.distance.maxDistance = maxDist;
					}
					break;

					case phys::eJointType::D6:
					{
						Str128 text;

						for (int i = 0; i < 6; ++i)
						{
							text.Format("d6 motion %d", i);
							glink->limit.d6.motion[i] = vt0.second.get<int>(text.c_str(), 0);
						}

						for (int i = 0; i < 6; ++i)
						{
							text.Format("d6 drive %d stiffness", i);
							glink->limit.d6.drive[i].stiffness = vt0.second.get<float>(text.c_str(), 0.f);

							text.Format("d6 drive %d damping", i);
							glink->limit.d6.drive[i].damping = vt0.second.get<float>(text.c_str(), 0.f);

							text.Format("d6 drive %d forcelimit", i);
							glink->limit.d6.drive[i].forceLimit = vt0.second.get<float>(text.c_str(), 0.f);

							text.Format("d6 drive %d accel", i);
							glink->limit.d6.drive[i].accel = vt0.second.get<bool>(text.c_str(), false);
						}

						glink->limit.d6.linear.value = vt0.second.get<float>("linear extent", 0.f);
						glink->limit.d6.linear.stiffness = vt0.second.get<float>("linear stiffness", 0.f);
						glink->limit.d6.linear.damping = vt0.second.get<float>("linear damping", 0.f);
						glink->limit.d6.linear.isLimit = glink->limit.d6.linear.stiffness != 0.f;

						glink->limit.d6.twist.lower = vt0.second.get<float>("angular lower", 0.f);
						glink->limit.d6.twist.upper = vt0.second.get<float>("angular upper", 0.f);
						glink->limit.d6.twist.isLimit = glink->limit.d6.twist.lower != 0.f;

						glink->limit.d6.swing.yAngle = vt0.second.get<float>("cone yAngle", 0.f);
						glink->limit.d6.swing.zAngle = vt0.second.get<float>("cone zAngle", 0.f);
						glink->limit.d6.swing.isLimit = glink->limit.d6.swing.yAngle != 0.f;

						const string linearVelStr = vt0.second.get<string>("linear drive velocity", "0 0 0");
						const Vector3 linearDriveVelocity = ParseVector3(linearVelStr);
						const string angularVelStr = vt0.second.get<string>("angular drive velocity", "0 0 0");
						const Vector3 angularDriveVelocity = ParseVector3(angularVelStr);

						*(Vector3*)glink->limit.d6.linearVelocity = linearDriveVelocity;
						*(Vector3*)glink->limit.d6.angularVelocity = angularDriveVelocity;
					}
					break;

					default:
						throw std::exception("joint type error");
					}

					outLink.push_back(glink);
				}//~for joint
			}//~joints
		}

	}
	catch (std::exception &e)
	{
		common::Str128 msg;
		msg.Format("Read Error!!, File [ %s ]\n%s"
			, fileName.c_str(), e.what());
		//MessageBoxA(NULL, msg.c_str(), "ERROR", MB_OK);
		return false;
	}

	outMap = gnodes;

	//------------------------------------------------------------------
	// update unique id
	map<int, int> ids; // original id, new id
	for (auto &gnode : outNode)
	{
		const int id = common::GenerateId();
		ids[gnode->id] = id;
		gnode->id = id;
	}

	// update iteration id
	for (auto &gnode : outNode)
	{
		if (gnode->iteration < 0)
			continue;
		gnode->iteration = ids[gnode->iteration];
	}

	outMap.clear();
	for (auto &gnode : outNode)
		outMap[gnode->id] = gnode;

	return true;
}
