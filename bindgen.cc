#include <clang-c/Index.h>

#include <iostream>
#include <string>
#include <vector>

namespace Bindgen {
  // function context
  int func_arg = 0;
  int func_numargs = 0;
  std::string func_name;
  std::vector<std::string> func_args;

  std::string getCursorKindName(CXCursorKind cursorKind)
  {
    CXString kindName = clang_getCursorKindSpelling(cursorKind);
    std::string result = clang_getCString(kindName);

    clang_disposeString(kindName);
    return result;
  }

  std::string getCursorSpelling(CXCursor cursor)
  {
    CXString cursorSpelling = clang_getCursorSpelling(cursor);
    std::string result = clang_getCString(cursorSpelling);

    clang_disposeString(cursorSpelling);
    return result;
  }

  std::string getCursorTypeSpelling(CXType cursorType)
  {
    CXString cursorTypeSpelling = clang_getTypeSpelling(cursorType);
    std::string result = clang_getCString(cursorTypeSpelling);

    clang_disposeString(cursorTypeSpelling);
    return result;
  }

  std::string getIntegerDecl(CXCursor cursor)
  {
    CXType cursorType = clang_getCursorType(cursor);

    std::string name = getCursorSpelling(cursor) ;
    std::string typeName = getCursorTypeSpelling(cursorType);

    return typeName + " " + name + " = info[" + std::to_string(func_arg) + "]->IntegerValue();\n";
  }

  std::string getUint32Decl(CXCursor cursor)
  {
    CXType cursorType = clang_getCursorType(cursor);

    std::string name = getCursorSpelling(cursor) ;
    std::string typeName = getCursorTypeSpelling(cursorType);

    return typeName + " " + name + " = info[" + std::to_string(func_arg) + "]->Uint32Value();\n";
  }

  std::string getNumberDecl(CXCursor cursor)
  {
    CXType cursorType = clang_getCursorType(cursor);

    std::string name = getCursorSpelling(cursor) ;
    std::string typeName = getCursorTypeSpelling(cursorType);

    return typeName + " " + name + " = info[" + std::to_string(func_arg) + "]->NumberValue();\n";
  }

  std::string getBoolDecl(CXCursor cursor)
  {
    CXType cursorType = clang_getCursorType(cursor);

    std::string name = getCursorSpelling(cursor) ;
    std::string typeName = getCursorTypeSpelling(cursorType);

    return typeName + " " + name + " = info[" + std::to_string(func_arg) + "]->BooleanValue();\n";
  }

  std::string getDataDecl(CXType type) {
    std::string typeName = getCursorTypeSpelling(type);

    switch (type.kind) {
    case CXType_Double:
      return "reinterpret_cast<" + typeName + " *>(GET_CONTENTS(info[" + std::to_string(func_arg) + "].As<v8::Float64Array>()));\n";
    case CXType_Float:
      return "reinterpret_cast<" + typeName + " *>(GET_CONTENTS(info[" + std::to_string(func_arg) + "].As<v8::Float32Array>()));\n";
    case CXType_Long:
      return "reinterpret_cast<" + typeName + " *>(GET_CONTENTS(info[" + std::to_string(func_arg) + "].As<v8::Int32Array>()));\n";
    case CXType_ULong:
      return "reinterpret_cast<" + typeName + " *>(GET_CONTENTS(info[" + std::to_string(func_arg) + "].As<v8::Uint32Array>()));\n";
    case CXType_Int:
      return "reinterpret_cast<" + typeName + " *>(GET_CONTENTS(info[" + std::to_string(func_arg) + "].As<v8::Int16Array>()));\n";
    case CXType_UInt:
      return "reinterpret_cast<" + typeName + " *>(GET_CONTENTS(info[" + std::to_string(func_arg) + "].As<v8::Uint16Array>()));\n";
    case CXType_UChar:
      return "reinterpret_cast<" + typeName + " *>(GET_CONTENTS(info[" + std::to_string(func_arg) + "].As<v8::Uint8Array>()));\n";
    case CXType_SChar:
      return "reinterpret_cast<" + typeName + " *>(GET_CONTENTS(info[" + std::to_string(func_arg) + "].As<v8::Int8Array>()));\n";
    default:
      break;
    }

    return "";
  }

  std::string getPointerDecl(CXCursor cursor)
  {
    CXType cursorType = clang_getCursorType(cursor);
    CXType pointerType = clang_getPointeeType(cursorType);

    std::string name = getCursorSpelling(cursor);
    std::string typeName = getCursorTypeSpelling(cursorType);
    std::string pointerTypeName = getCursorTypeSpelling(pointerType);

    switch (pointerType.kind) {
    case CXType_Double:
    case CXType_Float:
    case CXType_Long:
    case CXType_ULong:
    case CXType_Int:
    case CXType_UInt:
    case CXType_UChar:
    case CXType_SChar:
      return typeName + " " + name + " = " + getDataDecl(pointerType);
    case CXType_Complex: {
      CXType elementType = clang_getElementType(pointerType);
      std::string elementTypeName = getCursorTypeSpelling(elementType);

      switch(elementType.kind) {
        case CXType_Double:
        case CXType_Float:
        case CXType_Long:
        case CXType_ULong:
        case CXType_Int:
        case CXType_UInt:
        case CXType_UChar:
        case CXType_SChar:
          return typeName + " " + name + " = " + getDataDecl(cursorType);
        default:
          break;
      }
    }
    default:
      break;
    }

    return "";
  }

  CXChildVisitResult visitor(CXCursor cursor, CXCursor parent, CXClientData clientData)
  {
    CXSourceLocation location = clang_getCursorLocation(cursor);
    if (clang_Location_isFromMainFile(location) == 0) {
      return CXChildVisit_Continue;
    }

    CXCursorKind cursorKind = clang_getCursorKind(cursor);
    CXType cursorType = clang_getCursorType(cursor);

    unsigned int curLevel = *(reinterpret_cast<unsigned int*>(clientData));
    unsigned int nextLevel = curLevel + 1;

    std::string typeName = getCursorTypeSpelling(cursorType);
    std::string name = getCursorSpelling(cursor);

    if (name != "") {
      switch (cursorKind) {
      case CXCursor_FunctionDecl: {
        func_arg = 0;
        func_numargs = clang_getNumArgTypes(cursorType);
        func_name = name;

        std::cout << "void bindgen_" << name << "(v8::FunctionCallbackInfo<v8::Value>& info) {\n";
        if (func_numargs == 0) {
          std::cout << "  int i = " << func_name << "();\n";
          std::cout << "  info.GetReturnValue().Set(v8::Number::New(info.GetIsolate(), i));\n";
          std::cout << "}\n";
        }
        break;
      }
      case CXCursor_ParmDecl: {
        switch (cursorType.kind) {
        case CXType_LongLong:
        case CXType_ULongLong:
          std::cout << "  " + getIntegerDecl(cursor);
          break;
        case CXType_Long:
        case CXType_ULong:
        case CXType_Char32:
        case CXType_Int:
        case CXType_UInt:
        case CXType_Short:
        case CXType_UShort:
        case CXType_Char16:
        case CXType_UChar:
          std::cout << "  " + getUint32Decl(cursor);
          break;
        case CXType_Bool:
          std::cout << "  " + getBoolDecl(cursor);
          break;
        case CXType_LongDouble:
        case CXType_Double:
        case CXType_Float:
          std::cout << "  " + getNumberDecl(cursor);
          break;
        case CXType_Pointer: {
          std::cout << "  " + getPointerDecl(cursor);
        }
        default:
          break;
        }

        func_args.push_back(name);
        if (func_arg >= func_numargs - 1) {
          std::cout << "\n  int i = " << func_name << "(";
          for (int i = 0; i < func_args.size(); i++) {
            std::cout << func_args[i];
            if (i < func_args.size() - 1) {
              std::cout << ", ";
            }
          }

          std::cout << ");\n";
          std::cout << "  info.GetReturnValue().Set(v8::Number::New(info.GetIsolate(), i));\n";
          std::cout << "}\n\n";

          func_args.clear();
          func_name.clear();
        }

        func_arg++;
        break;
      }
      default:
        break;
      }
    }

    clang_visitChildren(cursor, visitor, &nextLevel);

    return CXChildVisit_Continue;
  }
}

int main(int argc, char** argv)
{
  if (argc < 2) {
    std::cout << "Usage ./bindgen <file>\n";
    return -1;
  }

  const char* args[] = { "-x", "c++" };

  CXIndex index = clang_createIndex(0, 0);
  CXTranslationUnit tu = clang_parseTranslationUnit(index, argv[1], args, 2, NULL, 0, 0);

  if (tu == NULL) {
    std::cout << "Error: Failed to parse header file.\n";
    return -1;
  }

  CXCursor rootCursor = clang_getTranslationUnitCursor(tu);

  unsigned int treeLevel = 0;

  clang_visitChildren(rootCursor, Bindgen::visitor, &treeLevel);

  clang_disposeTranslationUnit(tu);
  clang_disposeIndex(index);

  return 0;
}