//
// 2013-12-05, jjuiddong
// - GenoType parser
//
// 2020-02-02
//	- refactoring
//
#pragma once


#include "GenotypeScanner.h"

namespace evc { namespace genotype_parser {

	class cScanner;
	class cParser
	{
	public:
		cParser();
		virtual ~cParser();
		bool Parse( const string &fileName, bool isTrace=false);
		bool IsError() { return m_isErrorOccur; }
		void Clear();


	protected:
		//////////////////////////////////////////////////////////////////////////////////////
		// Rule~~
		//start -> expression_list;
		sExprList* program();

		//expression -> id ( string, id, vec3, material, [randshape,] connection-list )
		//	| string;
		sExpr* expression();

		//expression-list -> [ expression {, expression } ];
		sExprList* expression_list();

		// connection -> connection( id, quat, quat, vec3, limit, velocity, [randpos,] [randorient,] expression )
		sConnection* connection();

		// connection-list -> [ connection{, connection} ];
		sConnectionList* connection_list();

		//vec3 -> vec3( num, num, num ) ;
		Vector3 vec3();

		//pivot -> pivot( num, num, num ) ;
		Vector3 pivot();

		//transform -> transform( vec3, quat ) ;
		Transform transform();

		//quat -> quat(num, vec3);
		Quaternion quat();

		// drive -> drive(velocity(num), period(num))
		Vector2 drive();

		// limit -> limit(num, num, num)
		Vector4 limit();

		// twist limit -> twistlimit(num, num)
		Vector3 twistlimit();

		// swing limit -> swinglimit(num, num)
		Vector3 swinglimit();

		// density -> density(num)
		float density();

		// mass -> mass(num)
		float mass();

		// material -> material( material_arg )
		Vector3 material();

		// material_arg -> id | rgb
		Vector3 rgbValue();

		// velocity -> velocity(num)
		Vector3 velocity();

		// period -> period(num)
		float period();

		// randfield -> randshape | randpos | randorient
		Vector3 randField();

		// randpos -> randpos(num, num, num)
		Vector3 randpos();

		// randorient -> randorient(num, num, num)
		Vector3 randorient();

		// terminalOnly -> terminalOnly
		bool terminalonly();


		string number();
		int num();
		string id();
		string str();
		//////////////////////////////////////////////////////////////////////////////////////

		bool Match( Tokentype t );
		void SyntaxError( const char *msg, ... );
		void Build( sExpr *pmainExpr );
		void RemoveNoVisitExpression();


	public:
		string m_fileName;
		cScanner *m_scan;
		Tokentype m_token;
		bool m_isTrace;
		bool m_isErrorOccur;
		map<string, sExpr*> m_symTable;
		set<string> m_visit;
	};

}}
