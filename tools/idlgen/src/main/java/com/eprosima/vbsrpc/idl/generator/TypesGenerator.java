// Copyright 2016 Proyectos y Sistemas de Mantenimiento SL (eProsima).
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// ------------------------------------------------------------------
// Modification history:
// 2023-05 ~ 2025-05: idlgen optimization: support pubsub mode java/jni and RPC java/jni code generation, java typedef, ContainerPrintHelpers removal, production efficiency optimization, etc.
// ------------------------------------------------------------------

package com.eprosima.vbsrpc.idl.generator;

import com.eprosima.vbsrpc.idl.grammar.Context;
import com.eprosima.idl.generator.manager.TemplateManager;
import com.eprosima.idl.parser.tree.AnnotationDeclaration;
import com.eprosima.idl.parser.tree.Definition;
import com.eprosima.idl.parser.tree.Export;
import com.eprosima.idl.parser.tree.Interface;
import com.eprosima.idl.parser.tree.TypeDeclaration;
import com.eprosima.idl.parser.typecode.Kind;
import com.eprosima.log.ColorMessage;
import java.io.*;
import java.util.ArrayList;
import org.antlr.stringtemplate.StringTemplate;
import org.antlr.stringtemplate.StringTemplateGroup;
import java.util.Map;
import java.util.HashMap;

public class TypesGenerator {
    public TypesGenerator(TemplateManager tmanager, String outputDir, boolean replace, boolean isCj, boolean isRPC, boolean genHpp) {
        tmanager_ = tmanager;
        outputDir_ = outputDir;
        replace_ = replace;
        genhpp_ = genHpp;
        m_params_ = new HashMap<String, StringTemplateGroup>();
        m_proxy_name_ = new HashMap<String, String>();
        if (isCj) {
            if (isRPC) {
                m_params_.put("java_RPCJavaType", tmanager_.createStringTemplateGroup("RPCJavaType"));
                m_params_.put("java_Impl", tmanager_.createStringTemplateGroup("RPCImplJava"));
                m_params_.put("java_RpcClient", tmanager_.createStringTemplateGroup("RpcClientJava"));
                m_params_.put("java_RpcServerObj", tmanager_.createStringTemplateGroup("RpcServerObj"));
                m_params_.put("header_Impl", tmanager_.createStringTemplateGroup("RPCJNIImplHeader"));
                m_params_.put("source_Impl", tmanager_.createStringTemplateGroup("RPCJNIImplSource"));
                m_params_.put("header_RpcClient", tmanager_.createStringTemplateGroup("RPCJNIClientHeader"));
                m_params_.put("source_RpcClient", tmanager_.createStringTemplateGroup("RPCJNIClientSource"));
                m_params_.put("header_RpcServerObj", tmanager_.createStringTemplateGroup("RPCJNIServerObjHeader"));
                m_params_.put("source_RpcServerObj", tmanager_.createStringTemplateGroup("RPCJNIServerObjSource"));
                m_params_.put("header_type", tmanager_.createStringTemplateGroup("RPCJNItypeHeader"));
                m_params_.put("source_type", tmanager_.createStringTemplateGroup("RPCJNItypeSource"));
            } else {
                stg_ = tmanager_.createStringTemplateGroup("VBSJavaType");
            }

        } else {
            stg_ = tmanager_.createStringTemplateGroup("JavaType");
        }
    }

    /*
     * !
     *
     * @brief This function generates data types in Java.
     * It uses a context that was processed by the IDL parser.
     */
    public boolean generate(Context context, String packagDir, String packag, String libraryName,
            Map<String, String> extensions) {
        ArrayList<Definition> definitions = context.getDefinitions();

        // Create directory for module.
        String java_outputDir = packagDir + File.separator + "java";
        String cpp_outputDir = packagDir + File.separator + "cpp&h";
        File java_dir = new File(java_outputDir);
        File cpp_dir = new File(cpp_outputDir);

        if (!java_dir.exists()) {
            if (!java_dir.mkdir()) {
                System.out.println(
                        ColorMessage.error() + "Cannot create directory " + java_outputDir);
                return false;
            }
        }
        if (!cpp_dir.exists()) {
            if (!cpp_dir.mkdir()) {
                System.out.println(
                        ColorMessage.error() + "Cannot create directory " + cpp_outputDir);
                return false;
            }
        }
        boolean returnedValue = processDefinitions(context, definitions, packagDir, packag, extensions, "");
        if (returnedValue) {
            // Create gradle build script.
            StringTemplateGroup gradlestg = tmanager_.createStringTemplateGroup("gradle");
            StringTemplate gradlest = gradlestg.getInstanceOf("main");
            gradlest.setAttribute("name", libraryName);

            if (!writeFile(outputDir_ + "build.gradle", gradlest)) {
                System.out.println(ColorMessage.error() + "Cannot write file " + outputDir_ + "build.gradle");
                returnedValue = false;
            }
        }

        return returnedValue;
    }

    public boolean processDefinitions(Context context, ArrayList<Definition> definitions, String packagDir,
            String packag, Map<String, String> extensions, String moduleName) {
        if (definitions != null) {
            for (Definition definition : definitions) {
                if (definition.isIsModule()) {
                    com.eprosima.idl.parser.tree.Module module = (com.eprosima.idl.parser.tree.Module) definition;

                    if (!processDefinitions(context, module.getDefinitions(), packagDir,
                            packag, extensions, module.getName()))
                        return false;
                } else if (definition.isIsInterface()) {
                    Interface ifc = (Interface) definition;
                    m_proxy_name_.put(
                            context.getFilename() + "_" + (!moduleName.isEmpty() ? moduleName + "_" : "")
                                    + ifc.getName() + "Proxy",
                            (!moduleName.isEmpty() ? moduleName : "") + ifc.getName() + "Proxy");
                    // Create StringTemplate of the interface
                    for (Map.Entry<String, StringTemplateGroup> stg : m_params_.entrySet()) {
                        if(stg.getKey() == "java_RPCJavaType" || stg.getKey().contains("type"))
                            continue;
                        StringTemplate ifcst = stg.getValue().getInstanceOf("interface");
                        ifcst.setAttribute("ctx", context);
                        ifcst.setAttribute("parent", ifc.getParent());
                        ifcst.setAttribute("interface", ifc);
                        ifcst.setAttribute("module_name", (!moduleName.isEmpty() ? moduleName : null));
                        StringTemplate extensionst = null;
                        String extensionname = null;
                        if (extensions != null && (extensionname = extensions.get("interface")) != null) {
                            extensionst = stg.getValue().getInstanceOf(extensionname);
                            extensionst.setAttribute("ctx", context);
                            extensionst.setAttribute("parent", ifc.getParent());
                            extensionst.setAttribute("interface", ifc);
                            ifcst.setAttribute("extension", extensionst.toString());
                        }
                        if (processExports(context, ifc.getExports(), ifcst, extensions, stg.getValue(), moduleName)) {
                            // Save file.
                            StringTemplate st = stg.getValue().getInstanceOf("main");
                            st.setAttribute("ctx", context);
                            st.setAttribute("definitions", ifcst.toString());
                            st.setAttribute("package", (!packag.isEmpty() ? packag : null));
                            st.setAttribute("module_name", (!moduleName.isEmpty() ? moduleName : null));
                            if (stg.getKey().contains("header") || stg.getKey().contains("source")) {
                                st.setAttribute("interfacename", ifc.getName());
                            }
                            if (extensions != null && (extensionname = extensions.get("main")) != null) {
                                extensionst = stg.getValue().getInstanceOf(extensionname);
                                extensionst.setAttribute("ctx", context);
                                st.setAttribute("extension", extensionst.toString());
                            }
                            String last_name = "";
                            int lastUnderscoreIndex = stg.getKey().lastIndexOf('_');

                            if (lastUnderscoreIndex != -1) {
                                last_name = stg.getKey().substring(lastUnderscoreIndex);
                            } else {
                                System.out.println("No underscore found in the string.");
                            }

                            if (!writeFile(
                                    packagDir
                                            + ((stg.getKey().contains("java"))
                                                    ? File.separator + "java" + File.separator
                                                    : File.separator + "cpp&h" + File.separator)
                                            + ((stg.getKey().contains("java"))
                                                    ? ""
                                                    : context.getPackageUnder())
                                            + context.getFilename() + "_"
                                            + ((!moduleName.isEmpty() ? moduleName + "_" : ""))
                                            + ifc.getName()
                                            + last_name
                                            + (stg.getKey().contains("header") ? (genhpp_ ? ".hpp" : ".h")
                                                    : (stg.getKey().contains("source") ? ".cpp" : ".java")),
                                st)) {
                                System.out.println(
                                        ColorMessage.error() + "Cannot write file " + packagDir
                                            + ((stg.getKey().contains("java"))
                                                    ? File.separator + "java" + File.separator
                                                    : File.separator + "cpp&h" + File.separator)
                                            + context.getFilename() + "_"
                                            + ((!moduleName.isEmpty() ? moduleName + "_" : ""))
                                            + ifc.getName()
                                            + last_name
                                            + (stg.getKey().contains("header") ? (genhpp_ ? ".hpp" : ".h")
                                                    : (stg.getKey().contains("source") ? ".cpp" : ".java")));
                                return false;
                            }
                        } else
                            return false;
                    }

                } else if (definition.isIsTypeDeclaration()) {
                    TypeDeclaration typedecl = (TypeDeclaration) definition;
                    for (Map.Entry<String, StringTemplateGroup> stg : m_params_.entrySet()) {
                        // get StringTemplate of the structure
                        if(stg.getKey().contains("type") || stg.getKey().contains("Type")) {

                            StringTemplate typest = processTypeDeclaration(context, typedecl, extensions, stg.getValue(), moduleName);

                            if (typest != null) {
                                // Save file.

                                StringTemplate st = stg.getValue().getInstanceOf("main");
                                st.setAttribute("ctx", context);
                                st.setAttribute("definitions", typest.toString());
                                st.setAttribute("package", (!packag.isEmpty() ? packag : null));

                                StringTemplate extensionst = null;
                                String extensionname = null;
                                if (extensions != null && (extensionname = extensions.get("main")) != null) {
                                    extensionst = stg.getValue().getInstanceOf(extensionname);
                                    extensionst.setAttribute("ctx", context);
                                    st.setAttribute("extension", extensionst.toString());
                                }
                                if (!writeFile(
                                    packagDir
                                            + ((stg.getKey().contains("java"))
                                                    ? File.separator + "java" + File.separator
                                                    : File.separator + "cpp&h" + File.separator)
                                            + ((stg.getKey().contains("java"))
                                                    ? ""
                                                    : context.getPackageUnder())
                                            + context.getFilename() + "_"
                                            + ((!moduleName.isEmpty() ? moduleName + "_" : ""))
                                            + typedecl.getName()
                                            + (stg.getKey().contains("header") ? (genhpp_ ? ".hpp" : ".h")
                                                    : (stg.getKey().contains("source") ? ".cpp" : ".java")),
                                st)) {
                                System.out.println(
                                        ColorMessage.error() + "Cannot write file " + packagDir
                                            + ((stg.getKey().contains("java"))
                                                    ? File.separator + "java" + File.separator
                                                    : File.separator + "cpp&h" + File.separator)
                                            + context.getFilename() + "_"
                                            + ((!moduleName.isEmpty() ? moduleName + "_" : ""))
                                            + typedecl.getName()
                                            + (stg.getKey().contains("header") ? (genhpp_ ? ".hpp" : ".h")
                                                    : (stg.getKey().contains("source") ? ".cpp" : ".java")));
                                return false;
                            }
                            }
                        }

                    }
                } else if (definition.isIsAnnotation()) {
                    AnnotationDeclaration annotation = (AnnotationDeclaration) definition;

                    for (Map.Entry<String, StringTemplateGroup> stg : m_params_.entrySet()) {
                        // Create StringTemplate of the annotation
                        StringTemplate ifcst = stg.getValue().getInstanceOf("annotation");
                        ifcst.setAttribute("ctx", context);
                        // ifcst.setAttribute("parent", annotation.getParent());
                        ifcst.setAttribute("annotation", annotation);

                        StringTemplate extensionst = null;
                        String extensionname = null;
                        if (extensions != null && (extensionname = extensions.get("annotation")) != null) {
                            extensionst = stg.getValue().getInstanceOf(extensionname);
                            extensionst.setAttribute("ctx", context);
                            // extensionst.setAttribute("parent", annotation.getParent());
                            extensionst.setAttribute("annotation", annotation);
                            ifcst.setAttribute("extension", extensionst.toString());
                        }
                    }
                }
            }
        }

        return true;
    }

    public boolean processExports(Context context, ArrayList<Export> exports, StringTemplate ifcst,
            Map<String, String> extensions, StringTemplateGroup stg, String moduleName) {
        for (Export export : exports) {
            if (export.isIsTypeDeclaration()) {
                TypeDeclaration typedecl = (TypeDeclaration) export;

                // get StringTemplate of the structure
                StringTemplate typest = processTypeDeclaration(context, typedecl, extensions, stg, moduleName);

                if (typest != null) {
                    // Add type stringtemplate to interface stringtemplate.
                    ifcst.setAttribute("exports", typest.toString());
                }
            }
        }

        return true;
    }

    public StringTemplate processTypeDeclaration(Context context, TypeDeclaration typedecl,
            Map<String, String> extensions, StringTemplateGroup stg, String moduleName) {
        StringTemplate typest = null, extensionst = null;
        String extensionname = null;

        if (typedecl.getTypeCode().getKind() == Kind.KIND_STRUCT) {
            typest = stg.getInstanceOf("struct_type");
            typest.setAttribute("struct", typedecl.getTypeCode());
            context.setAllModuleList(context.getFilename() + "_" + typedecl.getTypeCode().getCScopednamenormal());
            if (!moduleName.isEmpty()) {
                typest.setAttribute("module_name", (!moduleName.isEmpty() ? moduleName : null));
            }
            // Get extension
            if (extensions != null && (extensionname = extensions.get("struct_type")) != null) {
                extensionst = stg.getInstanceOf(extensionname);
                extensionst.setAttribute("struct", typedecl.getTypeCode());
            }
        } else if (typedecl.getTypeCode().getKind() == Kind.KIND_UNION) {
            typest = stg.getInstanceOf("union_type");
            typest.setAttribute("union", typedecl.getTypeCode());

            // Get extension
            if (extensions != null && (extensionname = extensions.get("union_type")) != null) {
                extensionst = stg.getInstanceOf(extensionname);
                extensionst.setAttribute("union", typedecl.getTypeCode());
            }
        } else if (typedecl.getTypeCode().getKind() == Kind.KIND_ENUM) {
            typest = stg.getInstanceOf("enum_type");
            typest.setAttribute("enum", typedecl.getTypeCode());

            // Get extension
            if (extensions != null && (extensionname = extensions.get("enum_type")) != null) {
                extensionst = stg.getInstanceOf(extensionname);
                extensionst.setAttribute("enum", typedecl.getTypeCode());
            }
        } else if (typedecl.getTypeCode().getKind() == Kind.KIND_BITSET) {
            typest = stg.getInstanceOf("bitset_type");
            typest.setAttribute("bitset", typedecl.getTypeCode());

            // Get extension
            if (extensions != null && (extensionname = extensions.get("bitset_type")) != null) {
                extensionst = stg.getInstanceOf(extensionname);
                extensionst.setAttribute("bitset", typedecl.getTypeCode());
            }
        } else if (typedecl.getTypeCode().getKind() == Kind.KIND_BITMASK) {
            typest = stg.getInstanceOf("bitmask_type");
            typest.setAttribute("bitmask", typedecl.getTypeCode());

            // Get extension
            if (extensions != null && (extensionname = extensions.get("bitmask_type")) != null) {
                extensionst = stg.getInstanceOf(extensionname);
                extensionst.setAttribute("bitmask", typedecl.getTypeCode());
            }
        }

        if (typest != null) {
            // Generate extension
            if (extensionst != null) {
                extensionst.setAttribute("ctx", context);
                extensionst.setAttribute("parent", typedecl.getParent());
                typest.setAttribute("extension", extensionst.toString());
            }

            // Main stringtemplate
            typest.setAttribute("ctx", context);
            typest.setAttribute("parent", typedecl.getParent());
        }

        return typest;
    }

    private boolean writeFile(String file, StringTemplate template) {
        boolean returnedValue = false;

        try {
            File handle = new File(file);
            FileWriter fw;

            if (!handle.exists() || replace_) {
                fw = new FileWriter(file);
                String data = template.toString();
                fw.write(data, 0, data.length());
                fw.close();
            } else {
                System.out.println("INFO: " + file + " exists. Skipping.");
            }

            returnedValue = true;
        } catch (IOException e) {
            e.printStackTrace();
        }

        return returnedValue;
    }

    private TemplateManager tmanager_ = null;
    private StringTemplateGroup stg_ = null;
    private Map<String, StringTemplateGroup> m_params_;
    private Map<String, String> m_proxy_name_;
    private String outputDir_ = null;
    private boolean replace_ = false;
    private boolean genhpp_ = false;
}
