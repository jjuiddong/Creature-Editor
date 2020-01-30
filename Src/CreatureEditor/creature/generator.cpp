
#include "stdafx.h"
#include "generator.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/xml_parser.hpp>

namespace evc
{
	Vector4 ParseVector4(const string &str);
	Vector3 ParseVector3(const string &str);
	Vector2 ParseVector2(const string &str);

	bool Put_RigidActor(boost::property_tree::ptree &parent, phys::cRigidActor *actor);
	bool Put_Joint(boost::property_tree::ptree &parent, phys::cJoint *joint);
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
	, vector<phys::cRigidActor*> actors)
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
			auto it1 = std::find(actors.begin(), actors.end(), j->m_actor0);
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
					const float mass = vt0.second.get<float>("mass", 1.f);
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
					const string posStr = vt0.second.get<string>("jointpos", "0 0 0");
					const Vector3 pos = ParseVector3(posStr);
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
						throw std::exception("not found actor ptr");
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
							, sync0->actor, sync0->node->m_transform
							, sync1->actor, sync1->node->m_transform);

						cJointRenderer *jointRenderer = new cJointRenderer();
						jointRenderer->Create(joint);
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
							, sync0->actor, sync0->node->m_transform
							, sync1->actor, sync1->node->m_transform);

						if (isConeLimit)
						{
							joint->EnableConeLimit(true);
							joint->SetConeLimit(coneLimit);
						}

						cJointRenderer *jointRenderer = new cJointRenderer();
						jointRenderer->Create(joint);
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
						jointRenderer->Create(joint);
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
						jointRenderer->Create(joint);
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
						jointRenderer->Create(joint);
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
						jointRenderer->Create(joint);
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
