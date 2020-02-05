
#include "stdafx.h"
#include "GenotypeParser.h"

using namespace evc;
using namespace evc::genotype_parser;


cParser::cParser() 
{
	m_scan = new cScanner();
	m_isTrace = false;
	m_isErrorOccur = false;
}

cParser::~cParser()
{
	Clear();
}


bool cParser::Parse( const string &fileName
	, bool isTrace //=false
)
{
	if (!m_scan->LoadFile(fileName.c_str(), isTrace))
		return false;

	m_fileName = fileName;

	m_token = m_scan->GetToken();
	if (ENDFILE == m_token)
	{
		m_scan->Clear();
		return false;
	}

	{
		sExprList *root = program(); // parsing
		while (root) // remove all only sExprList list
		{
			sExprList *tmp = root;
			root = root->next;
			SAFE_DELETE(tmp);
		}
	}

	if (ENDFILE != m_token)
	{
		SyntaxError( " code ends before file " );
		m_scan->Clear();
		return false;
	}

	if (m_isErrorOccur)
		return false;

	//if (m_symTable.find("main") == m_symTable.end())
	//{
	//	SyntaxError( " Not Exist 'main' node" );
	//	m_scan->Clear();
	//	RemoveNoVisitExpression();
	//	return nullptr;
	//}
	//sExpr *mainExpr = m_symTable[ "main"];
	//m_visit.clear();
	//Build(mainExpr);
	//RemoveNoVisitExpression();
	//return mainExpr;

	// update expression ptr
	for (auto &kv : m_symTable)
	{
		sExpr *expr = kv.second;
		sConnectionList *con = expr->connection;
		while (con)
		{
			auto it = m_symTable.find(con->connect->exprName);
			if (m_symTable.end() == it)
				return false; // not found expression id
			con->connect->expr = it->second;
			con = con->next;
		}
	}

	return true;
}


// start -> expression;
sExprList* cParser::program()
{
	return expression_list();
}


//
// expression -> id ( string, id, vec3, material, density, [randshape,] [connection-list] )
// 	| string;
sExpr* cParser::expression()
{
	sExpr *pexpr = nullptr;
	if ((ID == m_token) && (LPAREN == m_scan->GetTokenQ(1)))
	{
		pexpr = new sExpr;
		pexpr->refCount = 0;
		pexpr->connection = nullptr;

		pexpr->id = id(); // shape
		Match(LPAREN);
		pexpr->id = str(); // overwrite
		Match(COMMA);
		pexpr->shape = id();
		Match(COMMA);
		pexpr->dimension = vec3();
		Match(COMMA);
		pexpr->material = material();
		Match(COMMA);
		pexpr->density = density();

		if (m_symTable.find(pexpr->id) == m_symTable.end())
		{
			m_symTable[ pexpr->id] = pexpr;
		}
		else
		{
			SyntaxError( "already exist expression = '%s' ", pexpr->id.c_str() );
		}

		//if (COMMA == m_token)
		//	Match(COMMA);
		//pexpr->randShape = randshape();

		if (COMMA == m_token)
			Match(COMMA);
		pexpr->connection = connection_list();

		Match(RPAREN);
	}
	//else if (STRING == m_token)
	//{
	//	const string Id = str();
	//	if (m_symTable.find(Id) == m_symTable.end())
	//	{
	//		SyntaxError( "not found expression = '%s' ", Id.c_str() );
	//	}
	//	else
	//	{
	//		pexpr = m_symTable[ Id];
	//	}
	//}
	return pexpr;
}


// expression-list -> [ expression {, expression } ];
sExprList* cParser::expression_list()
{
	sExpr *pexpr = expression();
	if (!pexpr)
		return nullptr;
	sExprList *plist = new sExprList;
	plist->expr = pexpr;

	if (COMMA == m_token)
		Match(COMMA);
	
	plist->next = expression_list();
	return plist;
}


// connection -> connection( id, vec3, vec3, vec3, vec3, transform, drive, limit, 
//					[twist limit,] [swing limit,] [terminalonly,] expression )
sConnection* cParser::connection()
{
	sConnection *con = nullptr;
	if (ID != m_token)
		return nullptr;

	const string tok = m_scan->GetTokenStringQ(0);
	if ((tok != "link") && (tok != "sensor"))
	{
		SyntaxError( "must declare %s -> 'joint / sensor' ", tok.c_str() );
		return nullptr;
	}

	con = new sConnection;
	con->conType = id(); // joint or sensor
	con->terminalOnly = false;

	Match(LPAREN);
	con->type = id();
	Match(COMMA);
	con->jointPos = vec3();
	Match(COMMA);
	con->jointAxis = vec3();
	Match(COMMA);
	con->pivot0 = pivot();
	Match(COMMA);
	con->pivot1 = pivot();
	Match(COMMA);
	con->conTfm0 = transform();
	Match(COMMA);
	con->conTfm1 = transform();

	if (COMMA == m_token)
		Match(COMMA);
	con->drive = drive();

	if (COMMA == m_token)
		Match(COMMA);
	con->limit = limit();

	if (COMMA == m_token)
		Match(COMMA);
	con->twistLimit = twistlimit();

	if (COMMA == m_token)
		Match(COMMA);
	con->swingLimit = swinglimit();

	if (COMMA == m_token)
		Match(COMMA);
	con->terminalOnly = terminalonly();

	if (COMMA == m_token)
		Match(COMMA);
	//con->expr = expression();
	con->exprName = str();

	Match(RPAREN);
	return con;
}


// connection-list -> [connection {, connection}];
sConnectionList* cParser::connection_list()
{
	sConnection *connct = connection();
	if (!connct)
		return nullptr;

	sConnectionList *plist = new sConnectionList;
	plist->connect = connct;

	if (COMMA == m_token)
		Match(COMMA);

	plist->next = connection_list();
	return plist;
}


// vec3 -> vec3( num, num, num ) ;
Vector3 cParser::vec3()
{
	Vector3 v;

	if (ID == m_token)
	{
		const string tok = m_scan->GetTokenStringQ(0);
		if (tok == "vec3")
		{
			Match(ID);
			Match(LPAREN);
			if (RPAREN != m_token)
			{
				v.x = (float)atof(number().c_str());
				Match(COMMA);
				v.y = (float)atof(number().c_str());
				Match(COMMA);
				v.z = (float)atof(number().c_str());
			}
			Match(RPAREN);
		}
		else
		{
			SyntaxError( "undeclare token %s, must declare 'vec3'\n", m_scan->GetTokenStringQ(0).c_str() );
		}
	}
	else
	{
		SyntaxError( "vec3 type need id string\n" );
	}

	return v;
}


// pivot -> pivot( num, num, num ) ;
Vector3 cParser::pivot()
{
	Vector3 v;

	if (ID == m_token)
	{
		const string tok = m_scan->GetTokenStringQ(0);
		if (tok == "pivot")
		{
			Match(ID);
			Match(LPAREN);
			if (RPAREN != m_token)
			{
				v.x = (float)atof(number().c_str());
				Match(COMMA);
				v.y = (float)atof(number().c_str());
				Match(COMMA);
				v.z = (float)atof(number().c_str());
			}
			Match(RPAREN);
		}
		else
		{
			SyntaxError("undeclare token %s, must declare 'pivot'\n", m_scan->GetTokenStringQ(0).c_str());
		}
	}
	else
	{
		SyntaxError("pivot type need id string\n");
	}

	return v;
}


//transform -> transform( vec3, quat ) ;
Transform cParser::transform()
{
	Transform tfm;

	if (ID == m_token)
	{
		const string tok = m_scan->GetTokenStringQ(0);
		if (tok == "transform")
		{
			Match(ID);
			Match(LPAREN);
			if (RPAREN != m_token)
			{
				tfm.pos = vec3();
				Match(COMMA);
				tfm.rot = quat();
			}
			Match(RPAREN);
		}
		else
		{
			SyntaxError("undeclare token %s, must declare 'transform'\n", m_scan->GetTokenStringQ(0).c_str());
		}
	}
	else
	{
		SyntaxError("transform type need id string\n");
	}

	return tfm;
}


// quat -> quat(x,y,z,w) | quat()
Quaternion cParser::quat()
{
	Quaternion quat;

	if (ID == m_token)
	{
		const string tok = m_scan->GetTokenStringQ(0);
		if (tok == "quat")
		{
			Match(ID);
			Match(LPAREN);
			if (RPAREN != m_token)
			{
				quat.x = (float)atof(number().c_str());
				Match(COMMA);
				quat.y = (float)atof(number().c_str());
				Match(COMMA);
				quat.z = (float)atof(number().c_str());
				Match(COMMA);
				quat.w = (float)atof(number().c_str());
			}
			Match(RPAREN);
		}
		else
		{
			SyntaxError( "undeclare token %s, must declare 'quat'\n", m_scan->GetTokenStringQ(0).c_str() );
		}
	}
	else
	{
		SyntaxError( "quat type need id string\n" );
	}

	return quat;
}


// drive -> drive(velocity(num), period(num))
Vector2 cParser::drive()
{
	Vector2 v;

	if (ID != m_token)
		return v;

	const string tok = m_scan->GetTokenStringQ(0);
	if (tok == "drive")
	{
		Match(ID);
		Match(LPAREN);
		v.x = velocity().x;
		if (COMMA == m_token)
		{
			Match(COMMA);
			v.y = period();
		}
		Match(RPAREN);
	}

	return v;
}


// limit -> conelimit(num, num)
//			| angularlimit (num, num)
//			| linearlimit (num, num, num, num)
//			| distancelimit (num, num)
//
Vector4 cParser::limit()
{
	Vector4 v;

	if (ID != m_token)
		return v;

	const string tok = m_scan->GetTokenStringQ(0);
	if ((tok == "conelimit") || (tok == "angularlimit") || (tok == "distancelimit"))
	{
		Match(ID);
		Match(LPAREN);
		v.x = (float)atof(number().c_str());
		Match(COMMA);
		v.y = (float)atof(number().c_str());
		Match(RPAREN);
	}
	else if (tok == "linearlimit")
	{
		Match(ID);
		Match(LPAREN);
		v.x = (float)atof(number().c_str());
		Match(COMMA);
		v.y = (float)atof(number().c_str());
		Match(COMMA);
		v.z = (float)atof(number().c_str());
		Match(COMMA);
		v.w = (float)atof(number().c_str());
		Match(RPAREN);
	}
	else
	{
		//SyntaxError( "undeclare token %s, must declare 'limit'\n", m_scan->GetTokenStringQ(0).c_str() );
	}

	return v;
}


// twist limit -> twistlimit(num, num)
Vector3 cParser::twistlimit()
{
	Vector3 v;

	if (ID != m_token)
		return v;

	const string tok = m_scan->GetTokenStringQ(0);
	if (tok == "twistlimit")
	{
		Match(ID);
		Match(LPAREN);
		v.x = (float)atof(number().c_str());
		Match(COMMA);
		v.y = (float)atof(number().c_str());
		Match(RPAREN);
	}

	return v;
}

// swing limit -> swinglimit(num, num)
Vector3 cParser::swinglimit()
{
	Vector3 v;

	if (ID != m_token)
		return v;

	const string tok = m_scan->GetTokenStringQ(0);
	if (tok == "swinglimit")
	{
		Match(ID);
		Match(LPAREN);
		v.x = (float)atof(number().c_str());
		Match(COMMA);
		v.y = (float)atof(number().c_str());
		Match(RPAREN);
	}

	return v;
}


// material -> material( rgbValue )
Vector3 cParser::material()
{
	if (ID != m_token)
		return Vector3(0.5f, 0.5f, 0.5f);

	Vector3 ret(0.5f, 0.5f, 0.5f);
	const string tok = m_scan->GetTokenStringQ(0);
	if (tok == "material")
	{
		Match(ID);
		Match(LPAREN);
		ret = rgbValue();
		Match(RPAREN);
	}
	else
	{
		SyntaxError( "undeclare token %s, must declare 'material'\n", m_scan->GetTokenStringQ(0).c_str() );
	}

	return ret;
}


// material_arg -> id | rgb
Vector3 cParser::rgbValue()
{
	Vector3 v;

	if (ID == m_token)
	{
		const string tok = m_scan->GetTokenStringQ(0);
		if (tok == "rgb")
		{
			Match(ID);
			Match(LPAREN);
			v.x = (float)atof(number().c_str());
			Match(COMMA);
			v.y = (float)atof(number().c_str());
			Match(COMMA);
			v.z = (float)atof(number().c_str());
			Match(RPAREN);
		}
		else if (tok == "grey")
		{
			Match(ID);
			v.x = 0.5f, v.y = 0.5f, v.z = 0.5f;			
		}
		else if (tok == "red")
		{
			Match(ID);
			v.x = 0.75f, v.y = 0, v.z = 0;
		}
		else if (tok == "green")
		{
			Match(ID);
			v.x = 0, v.y = 0.75f, v.z = 0;
		}
		else if (tok == "blue")
		{
			Match(ID);
			v.x = 0, v.y = 0, v.z = 0.75f;
		}
		else if (tok == "yellow")
		{
			Match(ID);
			v.x = 0.75f, v.y = 0.75f, v.z = 0;
		}
		else if (tok == "white")
		{
			Match(ID);
			v.x = 0.75f, v.y = 0.75f, v.z = 0.75f;
		}
		else
		{
			SyntaxError( "undeclare token %s, must declare 'rgb'\n", m_scan->GetTokenStringQ(0).c_str() );
		}
	}
	else
	{
		SyntaxError( "vec3 type need id string\n" );
	}

	return v;
}


// mass -> mass(num)
float cParser::mass()
{
	if (ID != m_token)
		return 0.f;

	float ret = 0.f;
	const string tok = m_scan->GetTokenStringQ(0);
	if (tok == "mass")
	{
		Match(ID);
		Match(LPAREN);
		ret = (float)atof(number().c_str());
		Match(RPAREN);
	}
	else
	{
		SyntaxError( "undeclare token %s, must declare 'mass'\n", m_scan->GetTokenStringQ(0).c_str() );
	}

	return ret;
}


// density -> density(num)
float cParser::density()
{
	if (ID != m_token)
		return 0.f;

	float ret = 0.f;
	const string tok = m_scan->GetTokenStringQ(0);
	if (tok == "density")
	{
		Match(ID);
		Match(LPAREN);
		ret = (float)atof(number().c_str());
		Match(RPAREN);
	}
	else
	{
		SyntaxError("undeclare token %s, must declare 'density'\n", m_scan->GetTokenStringQ(0).c_str());
	}

	return ret;
}


// velocity -> velocity( num )
Vector3 cParser::velocity()
{
	Vector3 v;

	if (ID != m_token)
		return v;

	const string tok = m_scan->GetTokenStringQ(0);
	if (tok == "velocity")
	{
		Match(ID);
		Match(LPAREN);
		v.x = (float)atof(number().c_str());
		Match(RPAREN);
	}
	else
	{
		// nothing
	}

	return v;
}


// period -> period(num)
float cParser::period()
{
	if (ID != m_token)
		return 0.f;

	float ret = 0.f;
	const string tok = m_scan->GetTokenStringQ(0);
	if (tok == "period")
	{
		Match(ID);
		Match(LPAREN);
		ret = (float)atof(number().c_str());
		Match(RPAREN);
	}
	else
	{
		// nothing
	}

	return ret;
}


// randfield -> randshape | randpos | randorient
Vector3 cParser::randField()
{
	Vector3 v;
	
	const string tok = m_scan->GetTokenStringQ(0);
	if (tok == "randpos")
	{
		v = randpos();
	}
	else if (tok == "randorient")
	{
		v = randorient();
	}
	//else if (tok == "randshape")
	//{
	//	v = randshape();
	//}

	return v;
}


// randshape -> randshape(num, num, num)
// 2014-01-13
// 2020-02-02, refactoring
//Vector3 cParser::randshape()
//{
//	Vector3 v;
//	v.x = v.y = v.z = 0.f;
//
//	if (ID != m_token)
//		return v;
//
//	const string tok = m_scan->GetTokenStringQ(0);
//	if (boost::iequals(tok, "randshape"))
//	{
//		Match(ID);
//		Match(LPAREN);
//		v.x = (float)atof(number().c_str());
//		Match(COMMA);
//		v.y = (float)atof(number().c_str());
//		Match(COMMA);
//		v.z = (float)atof(number().c_str());
//		Match(RPAREN);
//	}
//	else
//	{
//		// nothing
//	}
//
//	return v;
//}


// randpos -> randpos(num, num, num)
Vector3 cParser::randpos()
{
	Vector3 v;

	if (ID != m_token)
		return v;

	const string tok = m_scan->GetTokenStringQ(0);
	if (tok == "randpos")
	{
		Match(ID);
		Match(LPAREN);
		v.x = (float)atof(number().c_str());
		Match(COMMA);
		v.y = (float)atof(number().c_str());
		Match(COMMA);
		v.z = (float)atof(number().c_str());
		Match(RPAREN);
	}
	else
	{
		// nothing
	}

	return v;
}


// randorient -> randorient(num, num, num)
Vector3 cParser::randorient()
{
	Vector3 v;

	if (ID != m_token)
		return v;

	const string tok = m_scan->GetTokenStringQ(0);
	if (tok == "randorient")
	{
		Match(ID);
		Match(LPAREN);
		v.x = (float)atof(number().c_str());
		Match(COMMA);
		v.y = (float)atof(number().c_str());
		Match(COMMA);
		v.z = (float)atof(number().c_str());
		Match(RPAREN);
	}
	else
	{
		// nothing
	}

	return v;
}


// terminalOnly -> terminalOnly
bool cParser::terminalonly()
{
	if (ID != m_token)
		return false;

	const string tok = m_scan->GetTokenStringQ(0);
	if (tok == "terminalOnly")
	{
		Match(ID);
		return true;
	}
	return false;
}


string cParser::number()
{
	string str = m_scan->GetTokenStringQ(0);
	Match(NUM);
	return str;
}


int cParser::num()
{
	int n = atoi(m_scan->GetTokenStringQ(0).c_str());
	Match(NUM);
	return n;
}


string cParser::id()
{
	string str = m_scan->GetTokenStringQ(0);
	Match(ID);
	return str;
}


string cParser::str()
{
	string str = m_scan->GetTokenStringQ(0);
	Match(STRING);
	return str;
}


bool cParser::Match( Tokentype t )
{
	if( m_token == t )
	{
		m_token = m_scan->GetToken();
	}
	else
	{
		SyntaxError( "unexpected token -> " );
		printf( "\n" );
	}
	return true;
}


void cParser::SyntaxError( const char *msg, ... )
{
	m_isErrorOccur = true;
	char buf[ 256];
	va_list marker;
	va_start(marker, msg);
	vsprintf_s(buf, sizeof(buf), msg, marker);
	va_end(marker);

	stringstream ss;
	ss << "Syntax error at line " << m_fileName << " " << m_scan->GetLineNo() <<  ": " << buf << "\n";

	dbg::Logc(1, ss.str().c_str());
}


// increase reference counter
void cParser::Build( sExpr *pexpr )
{
	if (!pexpr)
		return;

	pexpr->refCount++;

	if (m_visit.find(pexpr->id) != m_visit.end())
		return; // already check

	m_visit.insert(pexpr->id);
	sConnectionList *pnode = pexpr->connection;
	while (pnode)
	{
		Build(pnode->connect->expr);
		pnode = pnode->next;
	}
}


// remove no visit expression node
void cParser::RemoveNoVisitExpression()
{
	set<string> rm;
	for (auto &kv : m_symTable)
	{
		if (kv.second->refCount <= 0)
		{
			RemoveExpressoin_OnlyExpr(kv.second);
			rm.insert(kv.first);
		}
	}
	for (auto &key : rm)
	{
		dbg::Logc(1, "remove not reference expression : %s", key.c_str());
		m_symTable.erase(key);
	}
}


void cParser::Clear()
{
	SAFE_DELETE(m_scan);

	for (auto &kv : m_symTable)
		RemoveExpressoin_OnlyExpr(kv.second);
	m_symTable.clear();
}
