
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
		{
			ptree shape;

			shape.put("name", "blank");
			shape.put<int>("id", a->m_id);
			shape.put<string>("type", phys::eRigidType::ToString(a->m_type));
			shape.put<string>("shape", phys::eShapeType::ToString(a->m_shape));

			// find local transform from joint
			Transform tfm;
			for (auto &j : a->m_joints)
			{
				if (j->m_actor0 == a)
				{
					tfm = j->m_actorLocal0;
					break;
				}
				if (j->m_actor1 == a)
				{
					tfm = j->m_actorLocal1;
					break;
				}
			}
			if (a->m_joints.empty()) // not found transform?
			{
				if (phys::sSyncInfo *sync = g_evc->m_sync->FindSyncInfo(a))
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

			shape.put<float>("mass", a->GetMass());
			shape.put<float>("angular damping", a->GetAngularDamping());
			shape.put<float>("linear damping", a->GetLinearDamping());
			shape.put<bool>("kinematic", a->IsKinematic());

			shapes.add_child("shape", shape);
		}

		// make joint
		ptree jts;
		for (auto &j : joints)
		{
			ptree joint;

			joint.put("type", phys::eJointType::ToString(j->m_type));
			joint.put("shape id0", j->m_actor0->m_id);
			joint.put("shape id1", j->m_actor1->m_id);

			v = j->m_origPos;
			text.Format("%f %f %f", v.x, v.y, v.z);
			joint.put("jointpos", text.c_str());

			q = j->m_rotRevolute;
			text.Format("%f %f %f %f", q.x, q.y, q.z, q.w);
			joint.put("revolute rot", text.c_str());

			v = j->m_pivots[0].dir;
			text.Format("%f %f %f", v.x, v.y, v.z);
			joint.put("pivot dir0", text.c_str());
			joint.put("pivot len0", j->m_pivots[0].len);

			v = j->m_pivots[1].dir;
			text.Format("%f %f %f", v.x, v.y, v.z);
			joint.put("pivot dir1", text.c_str());
			joint.put("pivot len1", j->m_pivots[1].len);

			joint.put<bool>("drive", j->IsDrive());
			joint.put("drive velocity", j->m_maxDriveVelocity);

			joint.put<bool>("cycle", j->m_isCycleDrive);
			joint.put("cycle period", j->m_cyclePeriod);
			joint.put("cycle accel", j->m_cycleDriveAccel);

			joint.put<bool>("cone limit", j->IsConeLimit());
			physx::PxJointLimitCone coneLimit = j->GetConeLimit();
			text.Format("%f %f %f", coneLimit.yAngle, coneLimit.zAngle, 0.01f);
			joint.put<string>("cone limit config", text.c_str());

			joint.put<bool>("angular limit", j->IsAngularLimit());
			physx::PxJointAngularLimitPair angularLimit = j->GetAngularLimit();
			text.Format("%f %f %f", angularLimit.lower, angularLimit.upper, 0.01f);
			joint.put<string>("angular limit config", text.c_str());

			jts.add_child("joint", joint);
		}

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
						if (phys::sSyncInfo *sync = g_evc->m_sync->FindSyncInfo(newId))
						{

						}
					}
					break;

					case phys::eShapeType::Capsule:
					{
						const Transform tfm(pos, q);
						newId = g_evc->m_sync->SpawnCapsule(renderer
							, tfm, dim.y, dim.x-dim.y, 1.f, true);
						if (phys::sSyncInfo *sync = g_evc->m_sync->FindSyncInfo(newId))
						{
							
						}
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
					const bool isDrive = vt0.second.get<bool>("drive");
					const float driveVelocity = vt0.second.get<float>("drive velocity");
					const bool isCycle = vt0.second.get<bool>("cycle", false);
					const float cyclePeriod = vt0.second.get<float>("cycle period", 0.f);
					const float cycleAccel = vt0.second.get<float>("cycle accel", 0.f);

					const bool isConeLimit = vt0.second.get<bool>("cone limit", false);
					const string coneLimitStr = vt0.second.get<string>("cone limit config", "0 0 0.01");
					const Vector3 tconf0 = ParseVector3(coneLimitStr);
					physx::PxJointLimitCone coneLimit(tconf0.x, tconf0.y, tconf0.z);

					const bool isAngularLimit = vt0.second.get<bool>("angular limit", false);
					const string angularLimitStr = vt0.second.get<string>("angular limit config", "0 0 0.01");
					const Vector3 tconf1 = ParseVector3(angularLimitStr);
					physx::PxJointAngularLimitPair angularLimit(tconf1.x, tconf1.y, tconf1.z);

					auto it0 = syncs.find(actorId0);
					auto it1 = syncs.find(actorId1);
					if ((syncs.end() == it0) || (syncs.end() == it1))
						throw std::exception("not found actor ptr");
					phys::sSyncInfo *sync0 = it0->second;
					phys::sSyncInfo *sync1 = it1->second;

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
						phys::cJoint *joint = new phys::cJoint();
						joint->CreateSpherical(g_global->m_physics
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
						// GetPivotWorldTransform
						const Transform tfm0 = sync0->node->m_transform;
						const Transform tfm1 = sync1->node->m_transform;
						const Vector3 localPos0 = pivotDir0 * tfm0.rot * pivotLen0;
						const Vector3 pivot0 = tfm0.pos + localPos0;
						const Vector3 localPos1 = pivotDir1 * tfm1.rot * pivotLen1;
						const Vector3 pivot1 = tfm1.pos + localPos1;
						const Vector3 revoluteAxis = Vector3(1,0,0) * revoluteQ;

						phys::cJoint *joint = new phys::cJoint();
						joint->CreateRevolute(g_global->m_physics
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
						break;
					case phys::eJointType::Distance:
						break;
					case phys::eJointType::D6:
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