%code requires {
  #include <memory>
  #include <string>
  #include <iostream>
  #include <inc/ast.hpp>
}

%{

#include <iostream>
#include <memory>
#include <string>
#include <inc/ast.hpp>

// 声明 lexer 函数和错误处理函数
int yylex();
void yyerror(std::unique_ptr<BaseAST> &ast, const char *s);

using namespace std;

%}

// 定义 parser 函数和错误处理函数的附加参数
// 我们需要返回一个字符串作为 AST, 所以我们把附加参数定义成字符串的智能指针
// 解析完成后, 我们要手动修改这个参数, 把它设置成解析得到的字符串
// %parse-param { std::unique_ptr<std::string> &ast }
%parse-param { std::unique_ptr<BaseAST> &ast }

// yylval 的定义, 我们把它定义成了一个联合体 (union)
// 因为 token 的值有的是字符串指针, 有的是整数
// 之前我们在 lexer 中用到的 str_val 和 int_val 就是在这里被定义的
// 至于为什么要用字符串指针而不直接用 string 或者 unique_ptr<string>?
// 请自行 STFW 在 union 里写一个带析构函数的类会出现什么情况
/*
%union {
  std::string *str_val;
  int int_val;
}
*/

// AST
%union
{
  std::string *str_val;
  int int_val;
  BaseAST *ast_val;
};


// lexer 返回的所有 token 种类的声明
// 注意 IDENT 和 INT_CONST 会返回 token 的值, 分别对应 str_val 和 int_val
%token INT RETURN LAND LOR CONST IF ELSE WHILE BREAK CONTINUE VOID
%token <str_val> IDENT REL EQ
%token <int_val> INT_CONST

// 非终结符的类型定义
%type <ast_val> FuncDef  Block Stmt Exp PrimaryExp UnaryExp AddExp MulExp LAndExp LOrExp RelExp EqExp Decl ConstDecl ConstDef ConstInitVal BlockItem LVal ConstExp ConstDefList BlockList VarDecl VarDef VarDefList InitVal FuncFParams FuncFParam FuncRParams UnitList Def DimList ConstInitVal_List InitVal_List OrdiStmt OpenStmt CloseStmt
%type <int_val> Number
%type <str_val> UnaryOp FuncType

%%

// 开始符, CompUnit ::= FuncDef, 大括号后声明了解析完成后 parser 要做的事情
// 之前我们定义了 FuncDef 会返回一个 str_val, 也就是字符串指针
// 而 parser 一旦解析完 CompUnit, 就说明所有的 token 都被解析了, 即解析结束了
// 此时我们应该把 FuncDef 返回的结果收集起来, 作为 AST 传给调用 parser 的函数
// $1 指代规则里第一个符号的返回值, 也就是 FuncDef 的返回值
CompUnit
  : UnitList {
    auto comp_unit = make_unique<CompUnitAST>();
    
    unique_ptr<CompUnitAST> unit_1 = unique_ptr<CompUnitAST>((CompUnitAST *) $1);

    comp_unit->defs.clear();

    
    for (int i = 0; i < unit_1->defs.size(); i++)
      comp_unit->defs.push_back(move(unit_1->defs[i]));
      
    // comp_unit->defs.push_back(unique_ptr<BaseAST>($2));
    
    ast = move(comp_unit);
  } 
  ;

UnitList
  : Def {
    auto ast = new CompUnitAST();
    ast->defs.clear();
    ast->defs.push_back(unique_ptr<BaseAST>($1));
    $$ = ast;
  }
  | UnitList Def {
    auto ast = new CompUnitAST();

    unique_ptr<CompUnitAST> unit_1 = unique_ptr<CompUnitAST>((CompUnitAST *) $1);

    ast->defs.clear();
    
    for (int i = 0; i < unit_1->defs.size(); i++)
      ast->defs.push_back(move(unit_1->defs[i]));
      
    ast->defs.push_back(unique_ptr<BaseAST>($2));
    
    $$ = ast;
  }
  ;

Def
  : 
  Decl {
    auto ast = new Def_AST();
    ast->def = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | FuncDef {
    auto ast = new Def_AST();
    ast->def = unique_ptr<BaseAST>($1);
    $$ = ast;
  };


// FuncDef ::= FuncType IDENT '(' ')' Block;
// 我们这里可以直接写 '(' 和 ')', 因为之前在 lexer 里已经处理了单个字符的情况
// 解析完成后, 把这些符号的结果收集起来, 然后拼成一个新的字符串, 作为结果返回
// $$ 表示非终结符的返回值, 我们可以通过给这个符号赋值的方法来返回结果
// 你可能会问, FuncType, IDENT 之类的结果已经是字符串指针了
// 为什么还要用 unique_ptr 接住它们, 然后再解引用, 把它们拼成另一个字符串指针呢
// 因为所有的字符串指针都是我们 new 出来的, new 出来的内存一定要 delete
// 否则会发生内存泄漏, 而 unique_ptr 这种智能指针可以自动帮我们 delete
// 虽然此处你看不出用 unique_ptr 和手动 delete 的区别, 但当我们定义了 AST 之后
// 这种写法会省下很多内存管理的负担

FuncDef
  : FuncType IDENT '(' ')' Block {
    auto ast = new FuncDefAST();
    ast->btype = *unique_ptr<string>($1);
    ast->ident = *unique_ptr<string>($2);
    ast->func_params = unique_ptr<BaseAST>((BaseAST *) NULL);
//    ast->arg_names.clear();
    ast->block = unique_ptr<BaseAST>($5);
    $$ = ast;
  }
  | FuncType IDENT '(' FuncFParams ')' Block {
    auto ast = new FuncDefAST();
    ast->btype = *unique_ptr<string>($1);
    ast->ident = *unique_ptr<string>($2);
    ast->func_params = unique_ptr<BaseAST>($4);

//    ast->arg_names.clear();
//    FuncFParam_list_AST *para_app = (FuncFParam_list_AST *) $4;
//    for (int i = 0; i < para_app->param_list.size(); i++)
//      ast->arg_names.push_back(para_app->arg_names[i]);

    ast->block = unique_ptr<BaseAST>($6);
    $$ = ast;
  };

// 同上, 不再解释
FuncType
  : INT {
    auto ast = new string("int");
    $$ = ast;
  }
  | VOID {
    auto ast = new string("void");
    $$ = ast;
  }
  ;

FuncFParams
  : FuncFParam {
    auto ast = new FuncFParam_list_AST();
    ast->param_list.clear();
    ast->param_list.push_back(unique_ptr<BaseAST>($1));

//    ast->arg_names.clear();
//    ast->arg_names.push_back(((FuncFParam_AST *)$1)->ident);
    $$ = ast;
  }
  | FuncFParams ',' FuncFParam {
    auto ast = new FuncFParam_list_AST();
    
    unique_ptr<FuncFParam_list_AST> para_app = unique_ptr<FuncFParam_list_AST>((FuncFParam_list_AST *) $1);

    ast->param_list.clear();
//    ast->arg_names.clear();
    
    for (int i = 0; i < para_app->param_list.size(); i++)
    {
      ast->param_list.push_back(move(para_app->param_list[i]));
//      ast->arg_names.push_back(para_app->arg_names[i]);
    }
      
    ast->param_list.push_back(unique_ptr<BaseAST>($3));
//    ast->arg_names.push_back(((FuncFParam_AST *)$3)->ident);

    $$ = ast;
  };

FuncFParam 
  : FuncType IDENT {
    auto ast = new FuncFParam_AST();
    ast->btype = *unique_ptr<string>($1);
    ast->ident = *unique_ptr<string>($2);
    $$ = ast;
  }
  | FuncType IDENT '[' ']' {
    auto ast = new FuncFParam_ptr_AST();
    ast->btype = *unique_ptr<string>($1);
    ast->ident = *unique_ptr<string>($2);
    ast->constexps.clear();
    $$ = ast;
  }
  | FuncType IDENT '[' ']' DimList {
    auto ast = new FuncFParam_ptr_AST();
    ast->btype = *unique_ptr<string>($1);
    ast->ident = *unique_ptr<string>($2);
    unique_ptr<Exp_List> dim_list = unique_ptr<Exp_List>((Exp_List *) $5);

    ast->constexps.clear();
    for (int i = 0; i < dim_list->exps.size(); i++)
      ast->constexps.push_back(move(dim_list->exps[i]));

    $$ = ast;
  };

FuncRParams
  : Exp {
    auto ast = new FuncRParams_AST();
    ast->exps.clear();
    ast->exps.push_back(unique_ptr<BaseAST>($1));
    $$ = ast;
  }
  | FuncRParams ',' Exp {
    auto ast = new FuncRParams_AST();
    
    unique_ptr<FuncRParams_AST> exp_app = unique_ptr<FuncRParams_AST>((FuncRParams_AST *) $1);

    ast->exps.clear();

    for (int i = 0; i < exp_app->exps.size(); i++)
      ast->exps.push_back(move(exp_app->exps[i]));

    ast->exps.push_back(unique_ptr<BaseAST>($3));

    $$ = ast;
  };

Block 
  : '{' BlockList '}' {
    auto ast = new BlockAST();
    unique_ptr<BlockAST> blk_list = unique_ptr<BlockAST>((BlockAST *) $2);
    
    ast->block_item.clear();
    
    for (int i = 0; i < blk_list->block_item.size(); i++)
      ast->block_item.push_back(move(blk_list->block_item[i]));
    
    $$ = ast;
    
  };

BlockList
  : {
    auto ast = new BlockAST();
    ast->block_item.clear();
    $$ = ast;
  }
  | BlockList BlockItem {
    auto ast = new BlockAST();
    unique_ptr<BlockAST> blk_list = unique_ptr<BlockAST>((BlockAST *) $1);
    
    ast->block_item.clear();
  
    for (int i = 0; i < blk_list->block_item.size(); i++)
      ast->block_item.push_back(move(blk_list->block_item[i]));
      
    ast->block_item.push_back(unique_ptr<BaseAST>($2));
    
    $$ = ast;
  };

BlockItem 
  : Stmt  {
    auto ast = new BlockItem2Stmt_AST();
    ast->stmt = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | Decl {
    auto ast = new BlockItem2Decl_AST();
    ast->decl = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

Stmt
  : OpenStmt {
    auto ast = $1;
    $$ = ast;
  }
  | CloseStmt {
    auto ast = $1;
    $$ = ast;
  };

// 不含 if 的 Stmt
OrdiStmt
  : RETURN Exp ';' {
    auto ast = new Stmt2Ret_AST();
    ast->exp = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  | RETURN ';' {
    auto ast = new Stmt2Ret_AST();
    ast->exp = unique_ptr<BaseAST>((BaseAST *) NULL);
    $$ = ast;
  }
  | LVal '=' Exp ';' {
      auto ast = new Stmt2Assign_AST();
      ast->lval = unique_ptr<BaseAST>($1);
      ast->exp = unique_ptr<BaseAST>($3);
      $$ = ast;
  }
  | Exp ';' {
    auto ast = new Stmt2Exp_AST();
    ast->exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | ';' {
    auto ast = new Stmt2Exp_AST();
    ast->exp = unique_ptr<BaseAST>((BaseAST *) NULL);
    $$ = ast;
  }
  | Block {
    auto ast = new Stmt2Blk_AST();
    ast->block = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | BREAK ';' {
    auto ast = new Stmt2Break_AST();
    $$ = ast;
  }
  | CONTINUE ';' {
    auto ast = new Stmt2Conti_AST();
    $$ = ast;
  };

// if 和 else 不完全对应的情况
OpenStmt
  : IF '(' Exp ')' Stmt {
    auto ast = new Stmt2Sing_IF_AST();
    ast->exp = unique_ptr<BaseAST>($3);
    ast->stmt = unique_ptr<BaseAST>($5);
    $$ = ast;
  }
  | IF '(' Exp ')' CloseStmt ELSE OpenStmt {
    auto ast = new Stmt2With_ELSE_AST();
    ast->exp = unique_ptr<BaseAST>($3);
    ast->then_stmt = unique_ptr<BaseAST>($5);
    ast->else_stmt = unique_ptr<BaseAST>($7);
    $$ = ast;
  }
  | WHILE '(' Exp ')' OpenStmt {
    auto ast = new Stmt2While_AST();
    ast->exp = unique_ptr<BaseAST>($3);
    ast->stmt = unique_ptr<BaseAST>($5);
    $$ = ast;
  };

// if 和 else 完全对应
CloseStmt
  : IF '(' Exp ')' CloseStmt ELSE CloseStmt {
    auto ast = new Stmt2With_ELSE_AST();
    ast->exp = unique_ptr<BaseAST>($3);
    ast->then_stmt = unique_ptr<BaseAST>($5);
    ast->else_stmt = unique_ptr<BaseAST>($7);
    $$ = ast;
  }
  | WHILE '(' Exp ')' CloseStmt {
    auto ast = new Stmt2While_AST();
    ast->exp = unique_ptr<BaseAST>($3);
    ast->stmt = unique_ptr<BaseAST>($5);
    $$ = ast;
  }
  | OrdiStmt {
    auto ast = $1;
    $$ = ast;
  };

Number
  : INT_CONST {
    auto ast = $1;
    $$ = ast;
  }
  ;

Exp
  : LOrExp {
    auto ast = new ExpAST();
    ast->loexp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

PrimaryExp
  : '(' Exp ')' {
    auto ast = new PExp2ExpAST();
    ast->exp = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  | Number {
    auto ast = new PExp2NumAST();
    ast->num = $1;
    $$ = ast;
  }
  | LVal {
    auto ast = new PExp2Lval_AST();
    ast->lval = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

UnaryExp
  : PrimaryExp {
    auto ast = new UExp2PExpAST();
    ast->pexp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | UnaryOp UnaryExp {
    auto ast = new UExp2UOp_UExpAST();
    ast->uop = *unique_ptr<string>($1);
    ast->uexp = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  | IDENT '(' ')' {
    auto ast = new UExp2Call_AST();
    ast->ident = *unique_ptr<string>($1);
    ast->exps.clear();
    $$ = ast;
  }
  | IDENT '(' FuncRParams ')' {
    auto ast = new UExp2Call_AST();
    ast->ident = *unique_ptr<string>($1);

    unique_ptr<FuncRParams_AST> params = unique_ptr<FuncRParams_AST>((FuncRParams_AST *) $3);

    ast->exps.clear();

    for (int i = 0; i < params->exps.size(); i++)
      ast->exps.push_back(move(params->exps[i]));

    $$ = ast;
  }
  ;

UnaryOp
  : '+' {
    auto ast = new string("+");
    $$ = ast;
  }
  | '-' {
    auto ast = new string("-");
    $$ = ast;
  }
  | '!' {
    auto ast = new string("!");
    $$ = ast;
  }
  ;

MulExp
  : UnaryExp {
    auto ast = new MExp2UExp_AST();
    ast->uexp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | MulExp '*' UnaryExp {
    auto ast = new MExp2MExp_UExp_AST();
    ast->mexp = unique_ptr<BaseAST>($1);
    ast->uexp = unique_ptr<BaseAST>($3);
    ast->op = "*";
    $$ = ast;
  }
  | MulExp '/' UnaryExp {
    auto ast = new MExp2MExp_UExp_AST();
    ast->mexp = unique_ptr<BaseAST>($1);
    ast->uexp = unique_ptr<BaseAST>($3);
    ast->op = "/";
    $$ = ast;
  }
  | MulExp '%' UnaryExp {
    auto ast = new MExp2MExp_UExp_AST();
    ast->mexp = unique_ptr<BaseAST>($1);
    ast->uexp = unique_ptr<BaseAST>($3);
    ast->op = "%";
    $$ = ast;
  }
  ;

AddExp
  : MulExp {
    auto ast = new AExp2MulExp_AST();
    ast->mexp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | AddExp '+' MulExp {
    auto ast = new AExp2AExp_MExp_AST();
    ast->aexp = unique_ptr<BaseAST>($1);
    ast->mexp = unique_ptr<BaseAST>($3);
    ast->op = "+";
    $$ = ast;
  }
  | AddExp '-' MulExp {
    auto ast = new AExp2AExp_MExp_AST();
    ast->aexp = unique_ptr<BaseAST>($1);
    ast->mexp = unique_ptr<BaseAST>($3);
    ast->op = "-";
    $$ = ast;
  }
  ;
  
RelExp
  : AddExp {
    auto ast = new RExp2AExp_AST();
    ast->aexp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | RelExp REL AddExp {
    auto ast = new RExp2R_rel_A_AST();
    ast->rexp = unique_ptr<BaseAST>($1);
    ast->aexp = unique_ptr<BaseAST>($3);
    ast->rel = *unique_ptr<string>($2);
    $$ = ast;
  };

EqExp
  : RelExp {
    auto ast = new EExp2RExp_AST();
    ast->rexp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | EqExp EQ RelExp {
    auto ast = new EExp2E_eq_R_AST();
    ast->eexp = unique_ptr<BaseAST>($1);
    ast->rexp = unique_ptr<BaseAST>($3);
    ast->rel = *unique_ptr<string>($2);
    $$ = ast;
  };

LAndExp
  : EqExp {
    auto ast = new LAExp2EExp_AST();
    ast->eexp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | LAndExp LAND EqExp {
    auto ast = new LAExp2L_and_E_AST();
    ast->laexp = unique_ptr<BaseAST>($1);
    ast->eexp = unique_ptr<BaseAST>($3);
    $$ = ast;
  };

LOrExp
  : LAndExp {
    auto ast = new LOExp2LAExp_AST();
    ast->laexp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | LOrExp LOR LAndExp {
    auto ast = new LOExp2L_or_LA_AST();
    ast->loexp = unique_ptr<BaseAST>($1);
    ast->laexp = unique_ptr<BaseAST>($3);
    $$ = ast;
  };

Decl
  : ConstDecl {
    auto ast = new Decl2Const_AST();
    ast->constdecl = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | VarDecl {
    auto ast = new Decl2Var_AST();
    ast->var_decl = unique_ptr<BaseAST>($1);
    $$ = ast;
  };

ConstDecl 
  : CONST FuncType ConstDefList ';' {
    auto ast = new ConstDecl_AST();
    ast->btype = *unique_ptr<string>($2);;
    
    unique_ptr<ConstDecl_AST> def_list = unique_ptr<ConstDecl_AST>((ConstDecl_AST *) $3);
    ast->constdefs.clear();
    
  
    for (int i = 0; i < def_list->constdefs.size(); i++)
      ast->constdefs.push_back(move(def_list->constdefs[i]));
    
    $$ = ast;
  };

ConstDefList
  : ConstDef {
    auto ast = new ConstDecl_AST();
    ast->constdefs.clear();
    ast->constdefs.push_back(unique_ptr<BaseAST>($1));
    $$ = ast;
  }
  | ConstDefList ',' ConstDef {
    auto ast = new ConstDecl_AST();
    unique_ptr<ConstDecl_AST> def_app = unique_ptr<ConstDecl_AST>((ConstDecl_AST *) $1);
    
    ast->constdefs.clear();
    
    for (int i = 0; i < def_app->constdefs.size(); i++)
      ast->constdefs.push_back(move(def_app->constdefs[i]));
      
    ast->constdefs.push_back(unique_ptr<BaseAST>($3));
    
    $$ = ast;
  };

VarDecl 
  : FuncType VarDefList ';' {
    auto ast = new VarDecl_AST();
    ast->btype = *unique_ptr<string>($1);
    
    unique_ptr<VarDecl_AST> def_list = unique_ptr<VarDecl_AST>((VarDecl_AST *) $2);
    ast->vardefs.clear();
    
  
    for (int i = 0; i < def_list->vardefs.size(); i++)
      ast->vardefs.push_back(move(def_list->vardefs[i]));
    
    $$ = ast;
  };

VarDefList
  : VarDef {
    auto ast = new VarDecl_AST();
    ast->vardefs.clear();
    ast->vardefs.push_back(unique_ptr<BaseAST>($1));
    $$ = ast;
  }
  | VarDefList ',' VarDef {
    auto ast = new VarDecl_AST();
    unique_ptr<VarDecl_AST> def_app = unique_ptr<VarDecl_AST>((VarDecl_AST *) $1);
    
    ast->vardefs.clear();
    
    for (int i = 0; i < def_app->vardefs.size(); i++)
      ast->vardefs.push_back(move(def_app->vardefs[i]));
      
    ast->vardefs.push_back(unique_ptr<BaseAST>($3));
    
    $$ = ast;
  };
/*
BType
  : INT {
    auto ast = new string("int");
    $$ = ast;
  };
*/

DimList
  : '[' Exp ']' {
    auto ast = new Exp_List();
    ast->exps.clear();
    ast->exps.push_back(unique_ptr<BaseAST>($2));
    $$ = ast;
  }
  | DimList '[' Exp ']' {
    auto ast = new Exp_List();
    unique_ptr<Exp_List> exp_list = unique_ptr<Exp_List>((Exp_List *) $1);

    ast->exps.clear();
    for (int i = 0; i < exp_list->exps.size(); i++)
      ast->exps.push_back(move(exp_list->exps[i]));
    ast->exps.push_back(unique_ptr<BaseAST>($3));

    $$ = ast;
  };

ConstDef
  : IDENT '=' ConstInitVal {
    auto ast = new ConstDef_AST();
    ast->ident = *unique_ptr<string>($1);
    ast->constinitval = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | IDENT DimList '=' ConstInitVal {
    auto ast = new ConstDef_Arr_AST();
    ast->ident = *unique_ptr<string>($1);
    ast->init_list = unique_ptr<BaseAST>($4);

    unique_ptr<Exp_List> dim_list = unique_ptr<Exp_List>((Exp_List *) $2);

    ast->constexps.clear();
    for (int i = 0; i < dim_list->exps.size(); i++)
      ast->constexps.push_back(move(dim_list->exps[i]));

    $$ = ast;
  };

VarDef
  : IDENT {
    auto ast = new VarDef_noninit_AST();
    ast->ident = *unique_ptr<string>($1);
    $$ = ast;
  }
  | IDENT '=' InitVal {
    auto ast = new VarDef_init_AST();
    ast->ident = *unique_ptr<string>($1);
    ast->initval = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | IDENT DimList {
    auto ast = new VarDef_Arr_noinit_AST();
    ast->ident = *unique_ptr<string>($1);
    
    unique_ptr<Exp_List> dim_list = unique_ptr<Exp_List>((Exp_List *) $2);

    ast->constexps.clear();
    for (int i = 0; i < dim_list->exps.size(); i++)
      ast->constexps.push_back(move(dim_list->exps[i]));
    $$ = ast;
  }
  | IDENT DimList '=' InitVal {
    auto ast = new VarDef_Arr_init_AST();
    ast->ident = *unique_ptr<string>($1);
    ast->init_list = unique_ptr<BaseAST>($4);

    unique_ptr<Exp_List> dim_list = unique_ptr<Exp_List>((Exp_List *) $2);

    ast->constexps.clear();
    for (int i = 0; i < dim_list->exps.size(); i++)
      ast->constexps.push_back(move(dim_list->exps[i]));

    $$ = ast;
  };

ConstInitVal 
  : ConstExp {
    auto ast = new ConstInitVal_AST();
    ast->const_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | '{' '}' {
    auto ast = new ConstInitVal_Arr_AST();
    ast->init_lists.clear();
    $$ = ast;
  }
  | '{' ConstInitVal_List '}' {
    auto ast = $2;
    $$ = ast;
  };

InitVal
  : Exp {
    auto ast = new InitVal_AST();
    ast->exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | '{' '}' {
    auto ast = new InitVal_Arr_AST();
    ast->init_lists.clear();
    $$ = ast;
  }
  | '{' InitVal_List '}' {
    auto ast = $2;
    $$ = ast;
  };

ConstInitVal_List
  : ConstInitVal {
    auto ast = new ConstInitVal_Arr_AST();
    ast->init_lists.clear();
    ast->init_lists.push_back(unique_ptr<BaseAST>($1));
    $$ = ast;
  }
  | ConstInitVal_List ',' ConstInitVal {
    auto ast = new ConstInitVal_Arr_AST();
    ast->init_lists.clear();

    unique_ptr<ConstInitVal_Arr_AST> app = unique_ptr<ConstInitVal_Arr_AST>((ConstInitVal_Arr_AST *) $1);

    for (int i = 0; i < app->init_lists.size(); i++)
      ast->init_lists.push_back(move(app->init_lists[i]));
    
    ast->init_lists.push_back(unique_ptr<BaseAST>($3));

    $$ = ast;
  };

InitVal_List
  : InitVal {
    auto ast = new InitVal_Arr_AST();
    ast->init_lists.clear();
    ast->init_lists.push_back(unique_ptr<BaseAST>($1));
    $$ = ast;
  }
  | InitVal_List ',' InitVal {
    auto ast = new InitVal_Arr_AST();
    ast->init_lists.clear();

    unique_ptr<InitVal_Arr_AST> app = unique_ptr<InitVal_Arr_AST>((InitVal_Arr_AST *) $1);

    for (int i = 0; i < app->init_lists.size(); i++)
      ast->init_lists.push_back(move(app->init_lists[i]));
    
    ast->init_lists.push_back(unique_ptr<BaseAST>($3));

    $$ = ast;
  }

LVal 
  : IDENT {
    auto ast = new LVal_AST();
    ast->ident = *unique_ptr<string>($1);
    $$ = ast;
  }
  | IDENT DimList {
    auto ast = new LVal_Arr_AST();
    ast->ident = *unique_ptr<string>($1);

    unique_ptr<Exp_List> dim_list = unique_ptr<Exp_List>((Exp_List *) $2);

    ast->exps.clear();
    for (int i = 0; i < dim_list->exps.size(); i++)
      ast->exps.push_back(move(dim_list->exps[i]));

    $$ = ast;
  };

ConstExp
  : Exp {
    auto ast = new ConstExp_AST();
    ast->exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  };


%%

// 定义错误处理函数, 其中第二个参数是错误信息
// parser 如果发生错误 (例如输入的程序出现了语法错误), 就会调用这个函数
void yyerror(unique_ptr<BaseAST> &ast, const char *s) {
  cerr << "error: " << s << endl;
}