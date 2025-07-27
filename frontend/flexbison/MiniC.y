%{
#include <cstdio>
#include <cstring>

// 词法分析头文件
#include "FlexLexer.h"

// bison生成的头文件
#include "BisonParser.h"

// 抽象语法树函数定义原型头文件
#include "AST.h"

#include "IntegerType.h"

// LR分析失败时所调用函数的原型声明
void yyerror(char * msg);

%}

// 联合体声明，用于后续终结符和非终结符号属性指定使用
%union {
    class ast_node * node;

    struct digit_int_attr integer_num;
    struct digit_real_attr float_num;
    struct var_id_attr var_id;
    struct type_attr type;
    int op_class;
};

// 文法的开始符号
%start  CompileUnit

// 指定文法的终结符号，<>可指定文法属性
// 对于单个字符的算符或者分隔符，在词法分析时可直返返回对应的ASCII码值，bison预留了255以内的值
// %token开始的符号称之为终结符，需要词法分析工具如flex识别后返回
// %type开始的符号称之为非终结符，需要通过文法产生式来定义
// %token或%type之后的<>括住的内容成为文法符号的属性，定义在前面的%union中的成员名字。
%token <integer_num> T_DIGIT
%token <float_num> T_FLOAT_DIGIT
%token <var_id> T_ID
%token <type> T_INT
%token <type> T_FLOAT

// 关键或保留字 一词一类 不需要赋予语义属性
%token T_RETURN
%token T_VOID
%token T_CONST

// 分隔符 一词一类 不需要赋予语义属性
%token T_SEMICOLON T_L_PAREN T_R_PAREN T_L_BRACE T_R_BRACE
%token T_COMMA T_L_BRACKET T_R_BRACKET

// 运算符
%token T_ASSIGN T_SUB T_ADD T_MUL T_DIV T_MOD

// 关系运算符
%token T_LT T_LE T_GT T_GE T_EQ T_NE

// 逻辑运算符
%token T_LOGICAL_AND T_LOGICAL_OR T_LOGICAL_NOT

// 关键字
%token T_IF T_ELSE T_WHILE T_BREAK T_CONTINUE

// 优先级设置
%nonassoc UMINUS
%left T_LOGICAL_OR
%left T_LOGICAL_AND
%left T_EQ T_NE
%left T_LT T_LE T_GT T_GE
%left T_ADD T_SUB
%left T_MUL T_DIV T_MOD


// 非终结符
// %type指定文法的非终结符号，<>可指定文法属性
%type <node> CompileUnit
%type <node> FuncDef
%type <node> Block
%type <node> BlockItemList
%type <node> BlockItem
%type <node> Statement
%type <node> Expr
%type <node> LogicalOrExp
%type <node> LogicalAndExp
%type <node> LVal
%type <node> VarDecl VarDeclExpr VarDef
%type <node> AddExp UnaryExp PrimaryExp MulExp RelExp
%type <node> RealParamList
%type <node> FormalParams FormalParam
%type <node> ArrayDims ArrayIndices
%type <node> ArrayInitList
%type <type> BasicType
%type <op_class> AddOp
%%

// 编译单元可包含若干个函数与全局变量定义。要在语义分析时检查main函数存在
// compileUnit: (funcDef | varDecl)* EOF;
// bison不支持闭包运算，为便于追加修改成左递归方式
// compileUnit: funcDef | varDecl | compileUnit funcDef | compileUnit varDecl
CompileUnit : FuncDef {

		// 创建一个编译单元的节点AST_OP_COMPILE_UNIT
		$$ = create_contain_node(ast_operator_type::AST_OP_COMPILE_UNIT, $1);

		// 设置到全局变量中
		ast_root = $$;
	}
	| VarDecl {

		// 创建一个编译单元的节点AST_OP_COMPILE_UNIT
		$$ = create_contain_node(ast_operator_type::AST_OP_COMPILE_UNIT, $1);
		ast_root = $$;
	}
	| CompileUnit FuncDef {

		// 把函数定义的节点作为编译单元的孩子
		$$ = $1->insert_son_node($2);
	}
	| CompileUnit VarDecl {
		// 把变量定义的节点作为编译单元的孩子
		$$ = $1->insert_son_node($2);
	}
	;

// 函数定义，目前支持整数返回类型，不支持形参
FuncDef : BasicType T_ID T_L_PAREN T_R_PAREN Block  {

		// 函数返回类型
		type_attr funcReturnType = $1;

		// 函数名
		var_id_attr funcId = $2;

		// 函数体节点即Block，即$5
		ast_node * blockNode = $5;

		// 形参结点没有，设置为空指针
		ast_node * formalParamsNode = nullptr;

		// 创建函数定义的节点，孩子有类型，函数名，语句块和形参(实际上无)
		// create_func_def函数内会释放funcId中指向的标识符空间，切记，之后不要再释放，之前一定要是通过strdup函数或者malloc分配的空间
		$$ = create_func_def(funcReturnType, funcId, blockNode, formalParamsNode);
	}
	| BasicType T_ID T_L_PAREN FormalParams T_R_PAREN Block {
		// 函数返回类型
		type_attr funcReturnType = $1;

		// 函数名
		var_id_attr funcId = $2;

		// 函数体节点即Block，即$6
		ast_node * blockNode = $6;

		// 形参结点，即$4
		ast_node * formalParamsNode = $4;

		// 创建函数定义的节点，孩子有类型，函数名，语句块和形参
		$$ = create_func_def(funcReturnType, funcId, blockNode, formalParamsNode);
	}
	| T_VOID T_ID T_L_PAREN T_R_PAREN Block {
		// 函数返回类型为void
		type_attr funcReturnType;
		funcReturnType.type = BasicType::TYPE_VOID;
		funcReturnType.lineno = yylineno;

		// 函数名
		var_id_attr funcId = $2;

		// 函数体节点即Block，即$5
		ast_node * blockNode = $5;

		// 形参结点没有，设置为空指针
		ast_node * formalParamsNode = nullptr;

		// 创建函数定义的节点，孩子有类型，函数名，语句块和形参(实际上无)
		$$ = create_func_def(funcReturnType, funcId, blockNode, formalParamsNode);
	}
	| T_VOID T_ID T_L_PAREN FormalParams T_R_PAREN Block {
		// 函数返回类型为void
		type_attr funcReturnType;
		funcReturnType.type = BasicType::TYPE_VOID;
		funcReturnType.lineno = yylineno;

		// 函数名
		var_id_attr funcId = $2;

		// 函数体节点即Block，即$6
		ast_node * blockNode = $6;

		// 形参结点，即$4
		ast_node * formalParamsNode = $4;

		// 创建函数定义的节点，孩子有类型，函数名，语句块和形参
		$$ = create_func_def(funcReturnType, funcId, blockNode, formalParamsNode);
	}
	;

// 形式参数列表
FormalParams : FormalParam {
		// 创建形参列表节点，并把当前的形参节点加入
		$$ = create_contain_node(ast_operator_type::AST_OP_FUNC_FORMAL_PARAMS, $1);
	}
	| FormalParams T_COMMA FormalParam {
		// 左递归增加形参
		$$ = $1->insert_son_node($3);
	}
	;

// 形式参数
FormalParam : BasicType T_ID {
		// 创建形参节点
		$$ = create_func_formal_param($1, $2);
	}
	| BasicType T_ID ArrayDims {
		// 创建数组形参节点
		var_id_attr id = $2;
		ast_node* id_node = ast_node::New(id);
		free(id.id);

		// 获取基本类型
		Type* baseType = typeAttr2Type($1);
		
		// 创建数组形参节点
		$$ = createArrayParamNode(baseType, id_node, $3);
	}
	| BasicType T_ID T_L_BRACKET T_R_BRACKET {
		// 创建简化形式的数组形参节点 int a[]
		var_id_attr id = $2;
		ast_node* id_node = ast_node::New(id);
		free(id.id);

		// 获取基本类型
		Type* baseType = typeAttr2Type($1);
		
		// 创建一个空的维度节点
		ast_node* empty_dims = create_contain_node(ast_operator_type::AST_OP_ARRAY_DIMS);
		
		// 添加一个维度，保留原始维度值
		digit_int_attr dim_attr;
		dim_attr.val = 0; // 使用一个合理的默认值
		dim_attr.lineno = yylineno;
		ast_node* dim_node = ast_node::New(dim_attr);
		empty_dims->insert_son_node(dim_node);
		
		// 创建数组形参节点
		$$ = createArrayParamNode(baseType, id_node, empty_dims);
	}
	| BasicType T_ID T_L_BRACKET T_R_BRACKET ArrayDims {
		// 创建多维数组形参节点 int a[][2][3]
		var_id_attr id = $2;
		ast_node* id_node = ast_node::New(id);
		free(id.id);

		// 获取基本类型
		Type* baseType = typeAttr2Type($1);
		
		// 创建一个维度节点，保留原始维度值
		ast_node* array_dims = create_contain_node(ast_operator_type::AST_OP_ARRAY_DIMS);
		
		// 添加一个维度，保留原始维度值
		digit_int_attr dim_attr;
		dim_attr.val = 0; // 使用一个合理的默认值
		dim_attr.lineno = yylineno;
		ast_node* dim_node = ast_node::New(dim_attr);
		array_dims->insert_son_node(dim_node);
		
		// 将其他维度添加到维度节点
		for (auto& son : $5->sons) {
			array_dims->insert_son_node(son);
		}
		
		// 创建数组形参节点
		$$ = createArrayParamNode(baseType, id_node, array_dims);
	}
	;

// 语句块的文法Block ： T_L_BRACE BlockItemList? T_R_BRACE
// 其中?代表可有可无，在bison中不支持，需要拆分成两个产生式
// Block ： T_L_BRACE T_R_BRACE | T_L_BRACE BlockItemList T_R_BRACE
Block : T_L_BRACE T_R_BRACE {
		// 语句块没有语句

		// 为了方便创建一个空的Block节点
		$$ = create_contain_node(ast_operator_type::AST_OP_BLOCK);
	}
	| T_L_BRACE BlockItemList T_R_BRACE {
		// 语句块含有语句

		// BlockItemList归约时内部创建Block节点，并把语句加入，这里不创建Block节点
		$$ = $2;
	}
	;

// 语句块内语句列表的文法：BlockItemList : BlockItem+
// Bison不支持正闭包，需修改成左递归形式，便于属性的传递与孩子节点的追加
// 左递归形式的文法为：BlockItemList : BlockItem | BlockItemList BlockItem
BlockItemList : BlockItem {
		// 第一个左侧的孩子节点归约成Block节点，后续语句可持续作为孩子追加到Block节点中
		// 创建一个AST_OP_BLOCK类型的中间节点，孩子为Statement($1)
		$$ = create_contain_node(ast_operator_type::AST_OP_BLOCK, $1);
	}
	| BlockItemList BlockItem {
		// 把BlockItem归约的节点加入到BlockItemList的节点中
		$$ = $1->insert_son_node($2);
	}
	;


// 语句块中子项的文法：BlockItem : Statement
// 目前只支持语句,后续可增加支持变量定义
BlockItem : Statement  {
		// 语句节点传递给归约后的节点上，综合属性
		$$ = $1;
	}
	| VarDecl {
		// 变量声明节点传递给归约后的节点上，综合属性
		$$ = $1;
	}
	;

// 变量声明语句
// 语法：varDecl: basicType varDef (T_COMMA varDef)* T_SEMICOLON
// 因Bison不支持闭包运算符，因此需要修改成左递归，修改后的文法为：
// VarDecl : VarDeclExpr T_SEMICOLON
// VarDeclExpr: BasicType VarDef | VarDeclExpr T_COMMA varDef
VarDecl : VarDeclExpr T_SEMICOLON {
		$$ = $1;
	}
	;

// 变量声明表达式，可支持逗号分隔定义多个
VarDeclExpr: BasicType VarDef {
		// 创建类型节点
		ast_node * type_node = create_type_node($1);
		Type * type = typeAttr2Type($1);

		// 创建变量定义节点
		ast_node * decl_node;
		
		// 判断VarDef是否为带初始化的变量定义，或者是数组定义
		if ($2->node_type == ast_operator_type::AST_OP_VAR_DEF_INIT) {
			// 如果是带初始化的变量定义，保持其节点结构
			decl_node = create_contain_node(ast_operator_type::AST_OP_VAR_DECL, type_node, $2);
		} else if ($2->node_type == ast_operator_type::AST_OP_ARRAY_DIMS) {
			// 如果是数组定义
			// 获取数组维度节点
			ast_node* dims_node = $2;
			// 获取ID节点（添加为数组维度的子节点）
			ast_node* id_node = dims_node->sons.back();
			// 移除ID节点
			dims_node->sons.pop_back();
			
			// 创建数组定义节点
			ast_node* array_def_node = createArrayDefNode(type, id_node, dims_node);
			
			// 创建变量声明节点
			decl_node = create_contain_node(ast_operator_type::AST_OP_VAR_DECL, type_node, array_def_node);
		} else {
			// 如果是普通变量定义，创建普通的变量声明节点
			decl_node = create_contain_node(ast_operator_type::AST_OP_VAR_DECL, type_node, $2);
		}
		
		decl_node->type = type;
		decl_node->is_const = false; // 非const变量

		// 创建变量声明语句，并加入第一个变量
		$$ = create_var_decl_stmt_node(decl_node);
	}
	| T_CONST BasicType VarDef {
		// 创建类型节点
		ast_node * type_node = create_type_node($2);
		Type * type = typeAttr2Type($2);

		// 创建变量定义节点
		ast_node * decl_node;
		
		// 判断VarDef是否为带初始化的变量定义，或者是数组定义
		if ($3->node_type == ast_operator_type::AST_OP_VAR_DEF_INIT) {
			// 如果是带初始化的变量定义，保持其节点结构
			decl_node = create_contain_node(ast_operator_type::AST_OP_VAR_DECL, type_node, $3);
		} else if ($3->node_type == ast_operator_type::AST_OP_ARRAY_DIMS) {
			// 如果是数组定义
			// 获取数组维度节点
			ast_node* dims_node = $3;
			// 获取ID节点（添加为数组维度的子节点）
			ast_node* id_node = dims_node->sons.back();
			// 移除ID节点
			dims_node->sons.pop_back();
			
			// 创建数组定义节点
			ast_node* array_def_node = createArrayDefNode(type, id_node, dims_node);
			
			// 创建变量声明节点
			decl_node = create_contain_node(ast_operator_type::AST_OP_VAR_DECL, type_node, array_def_node);
		} else {
			// 如果是普通变量定义，创建普通的变量声明节点
			decl_node = create_contain_node(ast_operator_type::AST_OP_VAR_DECL, type_node, $3);
		}
		
		decl_node->type = type;
		decl_node->is_const = true; // 标记为const变量

		// 创建变量声明语句，并加入第一个变量
		$$ = create_var_decl_stmt_node(decl_node);
	}
	| VarDeclExpr T_COMMA VarDef {
		// 获取类型，这里从VarDeclExpr获取类型，前面已经设置
		Type * type = $1->type;
		ast_node * type_node = ast_node::New(type);
		bool is_const = $1->sons[0]->is_const; // 传递const属性

		// 创建变量定义节点
		ast_node * decl_node;
		
		// 判断VarDef是否为带初始化的变量定义，或者是数组定义
		if ($3->node_type == ast_operator_type::AST_OP_VAR_DEF_INIT) {
			// 如果是带初始化的变量定义，保持其节点结构
			decl_node = create_contain_node(ast_operator_type::AST_OP_VAR_DECL, type_node, $3);
		} else if ($3->node_type == ast_operator_type::AST_OP_ARRAY_DIMS) {
			// 如果是数组定义
			// 获取数组维度节点
			ast_node* dims_node = $3;
			// 获取ID节点（添加为数组维度的子节点）
			ast_node* id_node = dims_node->sons.back();
			// 移除ID节点
			dims_node->sons.pop_back();
			
			// 创建数组定义节点
			ast_node* array_def_node = createArrayDefNode(type, id_node, dims_node);
			
			// 创建变量声明节点
			decl_node = create_contain_node(ast_operator_type::AST_OP_VAR_DECL, type_node, array_def_node);
		} else {
			// 如果是普通变量定义，创建普通的变量声明节点
			decl_node = create_contain_node(ast_operator_type::AST_OP_VAR_DECL, type_node, $3);
		}
		
		decl_node->type = type;
		decl_node->is_const = is_const; // 保持与前面变量相同的const属性

		// 插入到变量声明语句
		$$ = $1->insert_son_node(decl_node);
	}
	;

// 数组维度定义，例如 [10][20]
ArrayDims : T_L_BRACKET Expr T_R_BRACKET {
		// 创建数组维度节点
		$$ = create_contain_node(ast_operator_type::AST_OP_ARRAY_DIMS, $2);
	}
	| ArrayDims T_L_BRACKET Expr T_R_BRACKET {
		// 添加新的维度
		$$ = $1->insert_son_node($3);
	}
	;

// 数组下标访问，例如 [i][j]
ArrayIndices : T_L_BRACKET Expr T_R_BRACKET {
		// 创建数组索引节点
		$$ = create_contain_node(ast_operator_type::AST_OP_ARRAY_INDICES, $2);
	}
	| ArrayIndices T_L_BRACKET Expr T_R_BRACKET {
		// 添加新的索引
		$$ = $1->insert_son_node($3);
	}
	;

// 变量定义包含变量名，实际上还有初值，这里没有实现。
VarDef : T_ID {
		// 变量ID

		// 创建变量ID节点
		ast_node * id_node = ast_node::New(var_id_attr{$1.id, $1.lineno});

		// 对于字符型字面量的字符串空间需要释放，因词法用到了strdup进行了字符串复制
		free($1.id);
		
		// 创建不带初始化的变量定义节点
		$$ = id_node;
	}
	| T_ID ArrayDims {
		// 数组变量定义

		// 创建变量ID节点
		ast_node * id_node = ast_node::New(var_id_attr{$1.id, $1.lineno});

		// 对于字符型字面量的字符串空间需要释放，因词法用到了strdup进行了字符串复制
		free($1.id);
		
		// 创建数组变量定义节点
		$$ = createArrayDefNode(IntegerType::getTypeInt(), id_node, $2);
	}
	| T_ID T_ASSIGN Expr {
		// 变量ID和初始化表达式

		// 创建变量ID节点
		ast_node * id_node = ast_node::New(var_id_attr{$1.id, $1.lineno});

		// 对于字符型字面量的字符串空间需要释放，因词法用到了strdup进行了字符串复制
		free($1.id);

		// 创建带初始化的变量定义节点
		$$ = create_contain_node(ast_operator_type::AST_OP_VAR_DEF_INIT, id_node, $3);
	}
	| T_ID ArrayDims T_ASSIGN T_L_BRACE T_R_BRACE {
		// 空数组初始化 int a[4][2] = {}

		// 创建变量ID节点
		ast_node * id_node = ast_node::New(var_id_attr{$1.id, $1.lineno});

		// 对于字符型字面量的字符串空间需要释放，因词法用到了strdup进行了字符串复制
		free($1.id);
		
		// 创建空初始化列表节点
		ast_node * empty_init = create_contain_node(ast_operator_type::AST_OP_ARRAY_INIT_EMPTY);
		
		// 创建数组变量定义节点
		ast_node * array_def = createArrayDefNode(IntegerType::getTypeInt(), id_node, $2);
		
		// 添加空初始化列表作为子节点
		array_def->insert_son_node(empty_init);
		
		$$ = array_def;
	}
	| T_ID ArrayDims T_ASSIGN T_L_BRACE ArrayInitList T_R_BRACE {
		// 带初始化的数组定义 int a[4][2] = {1, 2, 3, 4, 5, 6, 7, 8}

		// 创建变量ID节点
		ast_node * id_node = ast_node::New(var_id_attr{$1.id, $1.lineno});

		// 对于字符型字面量的字符串空间需要释放，因词法用到了strdup进行了字符串复制
		free($1.id);
		
		// 创建数组变量定义节点
		ast_node * array_def = createArrayDefNode(IntegerType::getTypeInt(), id_node, $2);
		
		// 添加初始化列表作为子节点
		array_def->insert_son_node($5);
		
		$$ = array_def;
	}
	;

// 数组初始化列表
ArrayInitList : Expr {
		// 创建初始化列表节点，并添加第一个表达式
		ast_node * init_list = create_contain_node(ast_operator_type::AST_OP_ARRAY_INIT_LIST);
		if($1 == nullptr){
			ast_node * init = new ast_node(ast_operator_type::AST_OP_LEAF_LITERAL_UINT, IntegerType::getTypeInt(),-1);
			init ->integer_val = 0;
			ast_node * elem = create_contain_node(ast_operator_type::AST_OP_ARRAY_INIT_ELEM, init);
			init_list->insert_son_node(elem);
		}else {
			ast_node * elem = create_contain_node(ast_operator_type::AST_OP_ARRAY_INIT_ELEM, $1);
			init_list->insert_son_node(elem);
		}
	
		$$ = init_list;
	}
	| ArrayInitList T_COMMA Expr {
		// 添加更多的初始化表达式
		ast_node * elem = create_contain_node(ast_operator_type::AST_OP_ARRAY_INIT_ELEM, $3);
		$1->insert_son_node(elem);
		$$ = $1;
	}
	| T_L_BRACE ArrayInitList T_R_BRACE {
		// 嵌套初始化列表 例如 {{1, 2}, {3, 4}}
		//$$ = $2;
		// 修复1：嵌套列表作为单个元素
        ast_node* list_wrapper = create_contain_node(ast_operator_type::AST_OP_ARRAY_INIT_LIST, $2);
        $$ = list_wrapper;  // 返回包装后的元素节点
	}
	| T_L_BRACE T_R_BRACE {
		// 嵌套初始化列表 例如 {{1, 2}, {3, 4}}
		// 修复2：嵌套空列表作为单个元素
		ast_node * init_list = create_contain_node(ast_operator_type::AST_OP_ARRAY_INIT_LIST);
		ast_node * init = new ast_node(ast_operator_type::AST_OP_LEAF_LITERAL_UINT, IntegerType::getTypeInt(),-1);
		init ->integer_val = 0;
		ast_node * elem = create_contain_node(ast_operator_type::AST_OP_ARRAY_INIT_ELEM, init);
		init_list->insert_son_node(elem);
		
		ast_node* list_wrapper = create_contain_node(ast_operator_type::AST_OP_ARRAY_INIT_LIST, init_list);
        $$ = list_wrapper;  // 返回包装后的元素节点
	}
	| ArrayInitList T_COMMA T_L_BRACE ArrayInitList T_R_BRACE {
		// 嵌套初始化列表后跟更多元素 例如 {1, 2, {3}, {5}}
		$1->insert_son_node($4);
		$$ = $1;
	}
	| ArrayInitList T_COMMA T_L_BRACE  T_R_BRACE {
		// 嵌套初始化列表后跟更多元素 例如 {1, 2, {3}, {5}}
		// 修复2：嵌套空列表作为单个元素
		ast_node * init_list = create_contain_node(ast_operator_type::AST_OP_ARRAY_INIT_LIST);
		ast_node * init = new ast_node(ast_operator_type::AST_OP_LEAF_LITERAL_UINT, IntegerType::getTypeInt(),-1);
		init ->integer_val = 0;
		ast_node * elem = create_contain_node(ast_operator_type::AST_OP_ARRAY_INIT_ELEM, init);
		init_list->insert_son_node(elem);
		$1->insert_son_node(init_list);
		$$ = $1;
	}
	;

// 基本类型，目前只支持整型
BasicType: T_INT {
		$$ = $1;
	}
	| T_FLOAT {
		$$ = $1;
	}
	;

// 语句文法：statement:T_RETURN expr T_SEMICOLON | lVal T_ASSIGN expr T_SEMICOLON
// | block | expr? T_SEMICOLON
// 支持返回语句、赋值语句、语句块、表达式语句
// 其中表达式语句可支持空语句，由于bison不支持?，修改成两条
Statement : T_RETURN Expr T_SEMICOLON {
		// 返回语句

		// 创建返回节点AST_OP_RETURN，其孩子为Expr，即$2
		$$ = create_contain_node(ast_operator_type::AST_OP_RETURN, $2);
	}
	| T_RETURN T_SEMICOLON {
		// 不带表达式的返回语句

		// 创建不带表达式的返回节点AST_OP_RETURN
		$$ = create_contain_node(ast_operator_type::AST_OP_RETURN);
	}
	| LVal T_ASSIGN Expr T_SEMICOLON {
		// 赋值语句

		// 创建一个AST_OP_ASSIGN类型的中间节点，孩子为LVal($1)和Expr($3)
		$$ = create_contain_node(ast_operator_type::AST_OP_ASSIGN, $1, $3);
	}
	| Block {
		// 语句块

		// 内部已创建block节点，直接传递给Statement
		$$ = $1;
	}
	| Expr T_SEMICOLON {
		// 表达式语句

		// 内部已创建表达式，直接传递给Statement
		$$ = $1;
	}
	| T_SEMICOLON {
		// 空语句

		// 直接返回空指针，需要再把语句加入到语句块时要注意判断，空语句不要加入
		$$ = nullptr;
	}
	| T_IF T_L_PAREN Expr T_R_PAREN Statement {
		// if语句
		$$ = create_contain_node(ast_operator_type::AST_OP_IF, $3, $5);
	}
	| T_IF T_L_PAREN Expr T_R_PAREN Statement T_ELSE Statement {
		// if-else语句
		$$ = create_contain_node(ast_operator_type::AST_OP_IF_ELSE, $3, $5, $7);
	}
	| T_WHILE T_L_PAREN Expr T_R_PAREN Statement {
		// while语句
		$$ = create_contain_node(ast_operator_type::AST_OP_WHILE, $3, $5);
	}
	| T_BREAK T_SEMICOLON {
		// break语句
		$$ = create_contain_node(ast_operator_type::AST_OP_BREAK);
	}
	| T_CONTINUE T_SEMICOLON {
		// continue语句
		$$ = create_contain_node(ast_operator_type::AST_OP_CONTINUE);
	}
	;

// 顶层表达式
Expr : LogicalOrExp {
        $$ = $1;
    }
    ;

// 逻辑或表达式（||）
LogicalOrExp : LogicalAndExp {
        $$ = $1;
    }
    | LogicalOrExp T_LOGICAL_OR LogicalAndExp {
        $$ = create_contain_node(ast_operator_type::AST_OP_LOGICAL_OR, $1, $3);
    }
    ;

// 逻辑与表达式（&&）
LogicalAndExp : RelExp {
        $$ = $1;//$1;
    }
    | LogicalAndExp T_LOGICAL_AND RelExp {
        $$ = create_contain_node(ast_operator_type::AST_OP_LOGICAL_AND, $1, $3);
    }
    ;

// 关系表达式（<, <=, >, >=, ==, !=）
RelExp : AddExp {
        $$ = $1;
    }
    | RelExp T_LT AddExp {
        $$ = create_contain_node(ast_operator_type::AST_OP_LT, $1, $3);
    }
    | RelExp T_LE AddExp {
        $$ = create_contain_node(ast_operator_type::AST_OP_LE, $1, $3);
    }
    | RelExp T_GT AddExp {
        $$ = create_contain_node(ast_operator_type::AST_OP_GT, $1, $3);
    }
    | RelExp T_GE AddExp {
        $$ = create_contain_node(ast_operator_type::AST_OP_GE, $1, $3);
    }
    | RelExp T_EQ AddExp {
        $$ = create_contain_node(ast_operator_type::AST_OP_EQ, $1, $3);
    }
    | RelExp T_NE AddExp {
        $$ = create_contain_node(ast_operator_type::AST_OP_NE, $1, $3);
    }
    ;

// 加法表达式（+, -）
AddExp : MulExp {
        $$ = $1;
    }
	| AddExp T_ADD MulExp {
        $$ = create_contain_node(ast_operator_type::AST_OP_ADD, $1, $3);
    }
    | AddExp T_SUB MulExp {
        $$ = create_contain_node(ast_operator_type::AST_OP_SUB, $1, $3);
    }
    ;

// 加减运算符
AddOp: T_ADD {
		$$ = (int)ast_operator_type::AST_OP_ADD;
	}
	| T_SUB {
		$$ = (int)ast_operator_type::AST_OP_SUB;
	}
	;


// 乘法表达式（*, /, %）
MulExp : UnaryExp {
        $$ = $1;
    }
    | MulExp T_MUL UnaryExp {
        $$ = create_contain_node(ast_operator_type::AST_OP_MUL, $1, $3);
    }
    | MulExp T_DIV UnaryExp {
        $$ = create_contain_node(ast_operator_type::AST_OP_DIV, $1, $3);
    }
    | MulExp T_MOD UnaryExp {
        $$ = create_contain_node(ast_operator_type::AST_OP_MOD, $1, $3);
    }
    ;

// 目前一元表达式可以为基本表达式、函数调用，其中函数调用的实参可有可无
// 其文法为：unaryExp: primaryExp | T_ID T_L_PAREN realParamList? T_R_PAREN
// 由于bison不支持？表达，因此变更后的文法为：
// unaryExp: primaryExp | T_ID T_L_PAREN T_R_PAREN | T_ID T_L_PAREN realParamList T_R_PAREN
//一元表达式（-, !, 函数调用）
UnaryExp : PrimaryExp {
		// 基本表达式

		// 传递到归约后的UnaryExp上
		$$ = $1;
	}
	| T_ID T_L_PAREN T_R_PAREN {
		// 没有实参的函数调用

		// 创建函数调用名终结符节点
		ast_node * name_node = ast_node::New(std::string($1.id), $1.lineno);

		// 对于字符型字面量的字符串空间需要释放，因词法用到了strdup进行了字符串复制
		free($1.id);

		// 实参列表
		ast_node * paramListNode = nullptr;

		// 创建函数调用节点，其孩子为被调用函数名和实参，实参为空，但函数内部会创建实参列表节点，无孩子
		$$ = create_func_call(name_node, paramListNode);

	}
	| T_ID T_L_PAREN RealParamList T_R_PAREN {
		// 含有实参的函数调用

		// 创建函数调用名终结符节点
		ast_node * name_node = ast_node::New(std::string($1.id), $1.lineno);

		// 对于字符型字面量的字符串空间需要释放，因词法用到了strdup进行了字符串复制
		free($1.id);

		// 实参列表
		ast_node * paramListNode = $3;

		// 创建函数调用节点，其孩子为被调用函数名和实参，实参不为空
		$$ = create_func_call(name_node, paramListNode);
	}
	| T_SUB UnaryExp %prec UMINUS {
        // 单目运算符 - 的表达式
        // 创建单目运算符节点
        $$ = create_contain_node(ast_operator_type::AST_OP_NEG, $2);
  	}
	| T_ADD UnaryExp %prec UMINUS {
        // 单目运算符 + 的表达式，直接返回操作数
        $$ = $2;
    }	
	| T_LOGICAL_NOT UnaryExp {
		// 单目运算符 ! 的表达式
        $$ = create_contain_node(ast_operator_type::AST_OP_LOGICAL_NOT, $2);
    }
	;

// 基本表达式支持无符号整型字面量、带括号的表达式、具有左值属性的表达式
// 其文法为：primaryExp: T_L_PAREN expr T_R_PAREN | T_DIGIT | lVal
// 基本表达式（数字、变量、括号）
PrimaryExp :  T_L_PAREN Expr T_R_PAREN {
		// 带有括号的表达式
		$$ = $2;
	}
	| T_DIGIT {
        	// 无符号整型字面量

		// 创建一个无符号整型的终结符节点
		$$ = ast_node::New($1);
	}
	| LVal  {
		// 具有左值的表达式

		// 直接传递到归约后的非终结符号PrimaryExp
		$$ = $1;
	}
	| T_FLOAT_DIGIT {
		// 浮点数
		$$ = ast_node::New($1);
	}	
	;

// 实参表达式支持逗号分隔的若干个表达式
// 其文法为：realParamList: expr (T_COMMA expr)*
// 由于Bison不支持闭包运算符表达，修改成左递归形式的文法
// 左递归文法为：RealParamList : Expr | 左递归文法为：RealParamList T_COMMA expr
RealParamList : Expr {
		// 创建实参列表节点，并把当前的Expr节点加入
		$$ = create_contain_node(ast_operator_type::AST_OP_FUNC_REAL_PARAMS, $1);
	}
	| RealParamList T_COMMA Expr {
		// 左递归增加实参表达式
		$$ = $1->insert_son_node($3);
	}
	;

// 左值表达式，目前只支持变量名，实际上还有下标变量
LVal : T_ID {
		// 变量名终结符

		// 创建变量名终结符节点
		$$ = ast_node::New($1);

		// 对于字符型字面量的字符串空间需要释放，因词法用到了strdup进行了字符串复制
		free($1.id);
	}
	| T_ID ArrayIndices {
		// 数组变量访问

		// 创建变量名终结符节点
		ast_node * id_node = ast_node::New($1);

		// 对于字符型字面量的字符串空间需要释放，因词法用到了strdup进行了字符串复制
		free($1.id);

		// 创建数组访问节点
		$$ = createArrayAccessNode(id_node, $2);
	}
	;

%%


// 语法识别错误要调用函数的定义
void yyerror(char * msg)
{
    printf("Line %d: %s\n", yylineno, msg);
}

