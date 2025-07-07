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
// 2023-05 ~ 2025-05: Implemented pubsub Java/JNI multi-language, serialization and interface optimization, fixed typedef, build and efficiency issues.
// ------------------------------------------------------------------

package com.eprosima.vbscdr.idl.generator;

import com.eprosima.idl.context.Context;
import com.eprosima.idl.generator.manager.TemplateManager;
import com.eprosima.idl.parser.tree.AnnotationDeclaration;
import com.eprosima.idl.parser.tree.ConstDeclaration;
import com.eprosima.idl.parser.tree.Definition;
import com.eprosima.idl.parser.tree.Export;
import com.eprosima.idl.parser.tree.Interface;
import com.eprosima.idl.parser.tree.TypeDeclaration;
import com.eprosima.idl.parser.typecode.Kind;
import com.eprosima.log.ColorMessage;
import com.eprosima.idl.parser.typecode.TypeCode;
import java.io.*;
import java.util.ArrayList;
import java.util.Map;
import java.util.HashMap;
import org.antlr.stringtemplate.StringTemplate;
import org.antlr.stringtemplate.StringTemplateGroup;
import java.util.Set;
import java.util.HashSet;

public class TypesGenerator {
    public TypesGenerator(TemplateManager tmanager, String outputDir, boolean replace, boolean isCj, boolean isCPP, boolean useVbs, boolean genHpp, String fileName) {
        tmanager_ = tmanager;
        outputDir_ = outputDir;
        m_params_ = new HashMap<String, StringTemplateGroup>();
        replace_ = replace;
        genhpp_ = genHpp;
        fileName_ = fileName;
        idl_member_map_ = new HashMap<>();
        idl_member_map_.put("header", new HashSet<>());
        idl_member_map_.put("source", new HashSet<>());
        if (isCj) {
            isCJ_ = true;
            m_params_.put("", tmanager_.createStringTemplateGroup("VBSJavaType"));
            m_params_.put("java_PubSub", tmanager_.createStringTemplateGroup("CJPubSubClass"));
            m_params_.put("header_PubSubJNII", tmanager_.createStringTemplateGroup("CJPubSubJNIIHeader"));
            m_params_.put("source_PubSubJNII", tmanager_.createStringTemplateGroup("CJPubSubJNIISource"));

            if (!useVbs) {
                m_params_.put("java_Seq", tmanager_.createStringTemplateGroup("CJSeq"));
            }
        } else {
            if(!isCPP) {
                m_params_.put("", tmanager_.createStringTemplateGroup("JavaType"));
            } else {
                isCPP_ = isCPP;
                //m_params_.put("header_PubSubTypes", tmanager_.createStringTemplateGroup("DDSPubSubTypeHeader"));
                //m_params_.put("source_PubSubTypes", tmanager_.createStringTemplateGroup("DDSPubSubTypeSource"));
            }
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
        context_ = context;
        boolean returnedValue = processDefinitions(context, definitions, packagDir, packag, extensions, "", packagDir);
        if (returnedValue && !isCJ_) {
            // Create gradle build script.
            StringTemplateGroup gradlestg = tmanager_.createStringTemplateGroup("gradle");
            StringTemplate gradlest = gradlestg.getInstanceOf("main");
            gradlest.setAttribute("name", libraryName);

            if (!writeFile(outputDir_ + "build.gradle", gradlest, false)) {
                System.out.println(ColorMessage.error() + "Cannot write file " + outputDir_ + "build.gradle");
                returnedValue = false;
            }

            if (!appendTail(packagDir + fileName_ + "PubSubTypes" + (genhpp_ ? ".hpp" : ".h"))) {
                System.out.println(ColorMessage.error() + "Add " + packagDir
                        + fileName_ + "PubSubTypes" + (genhpp_ ? ".hpp" : ".h") + " tail error");
                return false;
            }
        }
        return returnedValue;
    }

    public boolean processDefinitions(Context context, ArrayList<Definition> definitions, String packagDir,
            String packag, Map<String, String> extensions, String moduleName, String oldPackagDir) {
        if (definitions != null) {
            for (Definition definition : definitions) {
                if (definition.isIsModule()) {
                    com.eprosima.idl.parser.tree.Module module = (com.eprosima.idl.parser.tree.Module) definition;

                    // Create directory for module.
                    String outputDir = packagDir + module.getName();
                    File dir = new File(outputDir);

                    if(!isCPP_) {
                        if (!dir.exists()) {
                            if (!dir.mkdir()) {
                                System.out.println(
                                        ColorMessage.error() + "Cannot create directory for module " + module.getName());
                                return false;
                            }
                        }
                    }

                    if (!processDefinitions(context, module.getDefinitions(), outputDir + File.separator,
                            packag + "." + module.getName(), extensions, (moduleName.isEmpty() ? "" : moduleName + "::" )+ module.getName(), oldPackagDir))
                        return false;
                } else if (definition.isIsInterface()) {
                    for (Map.Entry<String, StringTemplateGroup> stg : m_params_.entrySet()) {
                        if(stg.getKey().contains("header") || stg.getKey().contains("source"))
                            continue;
                        Interface ifc = (Interface) definition;

                        // Create StringTemplate of the interface
                        StringTemplate ifcst = stg.getValue().getInstanceOf("interface");
                        ifcst.setAttribute("ctx", context);
                        ifcst.setAttribute("parent", ifc.getParent());
                        ifcst.setAttribute("interface", ifc);

                        StringTemplate extensionst = null;
                        String extensionname = null;
                        if (extensions != null && (extensionname = extensions.get("interface")) != null) {
                            extensionst = stg.getValue().getInstanceOf(extensionname);
                            extensionst.setAttribute("ctx", context);
                            extensionst.setAttribute("parent", ifc.getParent());
                            extensionst.setAttribute("interface", ifc);
                            ifcst.setAttribute("extension", extensionst.toString());
                        }

                        if (processExports(context, ifc.getExports(), ifcst, extensions, stg.getValue(), "")) {
                            // Save file.
                            StringTemplate st = stg.getValue().getInstanceOf("main");
                            st.setAttribute("ctx", context);
                            st.setAttribute("definitions", ifcst.toString());
                            st.setAttribute("package", (!packag.isEmpty() ? packag : null));

                            if (extensions != null && (extensionname = extensions.get("main")) != null) {
                                extensionst = stg.getValue().getInstanceOf(extensionname);
                                extensionst.setAttribute("ctx", context);
                                st.setAttribute("extension", extensionst.toString());
                            }
                            if (!writeFile(packagDir + ifc.getName() + stg.getKey() + ".java", st, false)) {
                                System.out.println(
                                        ColorMessage.error() + "Cannot write file " + packagDir + ifc.getName() + ".java");
                                return false;
                            }
                        } else
                            return false;
                    }
                } else if (definition.isIsTypeDeclaration()) {
                    for (Map.Entry<String, StringTemplateGroup> stg : m_params_.entrySet()) {
                        TypeDeclaration typedecl = (TypeDeclaration) definition;
                        // get StringTemplate of the structure
                        StringTemplate typest = null;

                        typest = processTypeDeclaration(context, typedecl, extensions, stg.getValue(), "", false);

                        if (typest != null) {
                            // Save file.
                            StringTemplate st = stg.getValue().getInstanceOf("main");
                            st.setAttribute("ctx", context);
                            st.setAttribute("definitions", typest.toString());
                            if(!(stg.getKey().contains("header") || stg.getKey().contains("source"))) {
                                st.setAttribute("package", (!packag.isEmpty() ? packag : null));
                            }

                            StringTemplate extensionst = null;
                            String extensionname = null;
                            if (extensions != null && (extensionname = extensions.get("main")) != null) {
                                extensionst = stg.getValue().getInstanceOf(extensionname);
                                extensionst.setAttribute("ctx", context);
                                st.setAttribute("extension", extensionst.toString());
                            }
                            if (typedecl.getName().equals(context.getFilename())) {
                                context.setTargetStructScop(typedecl.getScope());
                            }
                            String last_name = "";
                            int lastUnderscoreIndex = stg.getKey().lastIndexOf('_');

                            if (lastUnderscoreIndex != -1) {
                                last_name = stg.getKey().substring(lastUnderscoreIndex + 1);
                            }
                            if(stg.getKey().contains("header") || stg.getKey().contains("source")) {
                                if (!writeFile(outputDir_
                                            + TypeCode.javapackage.replace('.', '_')
                                            + typedecl.getCScopedname()
                                            + last_name
                                            + (stg.getKey().contains("header") ? (genhpp_ ? ".hpp" : ".h")
                                                    : ".cxx"),
                                st, stg.getKey().contains("header") ? header_first_write_ : source_first_write_)) {
                                    System.out.println(ColorMessage.error() + "Cannot write file " + outputDir_
                                            + TypeCode.javapackage.replace('.', '_')
                                            + typedecl.getCScopedname()
                                            + last_name
                                            + (stg.getKey().contains("header") ? (genhpp_ ? ".hpp" : ".h")
                                                    : ".cxx"));
                                    return false;
                                }
                            } else {
                                if (!writeFile(packagDir + typedecl.getName() + last_name + ".java", st, false)) {
                                    System.out.println(ColorMessage.error() + "Cannot write file " + packagDir
                                            + typedecl.getName() + ".java");
                                    return false;
                                }
                            }
                        }
                    }
                } else if (definition.isIsAnnotation()) {
                    for (Map.Entry<String, StringTemplateGroup> stg : m_params_.entrySet()) {
                        AnnotationDeclaration annotation = (AnnotationDeclaration) definition;

                        // Create StringTemplate of the annotation
                        StringTemplate ifcst = stg.getValue().getInstanceOf("annotation");
                        ifcst.setAttribute("ctx", context);
                        // ifcst.setAttribute("parent", annotation.getParent());
                        ifcst.setAttribute("annotation", annotation);
                        if (isCPP_) {
                            ifcst.setAttribute("isfirst", (first_obj_ ? "1" : null));
                        }

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
                } else if (definition.isIsConstDeclaration()) {
                    if (isCPP_) {
                        for (Map.Entry<String, StringTemplateGroup> stg : m_params_.entrySet()) {

                            StringTemplate st = stg.getValue().getInstanceOf("const_decl");
                            st.setAttribute("ctx", context);
                            st.setAttribute("isfirst", (first_obj_ ? "1" : null));
                            if(stg.getKey().contains("header") || stg.getKey().contains("source")) {
                                String last_name = "";
                                int lastUnderscoreIndex = stg.getKey().lastIndexOf('_');

                                if (lastUnderscoreIndex != -1) {
                                    last_name = stg.getKey().substring(lastUnderscoreIndex + 1);
                                } else {
                                    System.out.println("No underscore found in the string.");
                                }
                                if (!writeFile(oldPackagDir
                                            + fileName_
                                            + last_name
                                            + (stg.getKey().contains("header") ? (genhpp_ ? ".hpp" : ".h")
                                                    : ".cxx"),
                                st, stg.getKey().contains("header") ? header_first_write_ : source_first_write_)) {
                                    System.out.println(ColorMessage.error() + "Cannot write file " + oldPackagDir
                                            + fileName_
                                            + last_name
                                            + (stg.getKey().contains("header") ? (genhpp_ ? ".hpp" : ".h")
                                                    : ".cxx"));
                                    return false;
                                }
                            }
                        }
                    }
                } else if (definition.isIsTypeDeclaration()) {
                    if (isCPP_) {
                        for (Map.Entry<String, StringTemplateGroup> stg : m_params_.entrySet()) {

                            StringTemplate st = stg.getValue().getInstanceOf("typedef_decl");
                            st.setAttribute("ctx", context);
                            st.setAttribute("isfirst", (first_obj_ ? "1" : null));
                            if(stg.getKey().contains("header") || stg.getKey().contains("source")) {
                                String last_name = "";
                                int lastUnderscoreIndex = stg.getKey().lastIndexOf('_');

                                if (lastUnderscoreIndex != -1) {
                                    last_name = stg.getKey().substring(lastUnderscoreIndex + 1);
                                } else {
                                    System.out.println("No underscore found in the string.");
                                }
                                if (!writeFile(oldPackagDir
                                            + fileName_
                                            + last_name
                                            + (stg.getKey().contains("header") ? (genhpp_ ? ".hpp" : ".h")
                                                    : ".cxx"),
                                st, stg.getKey().contains("header") ? header_first_write_ : source_first_write_)) {
                                    System.out.println(ColorMessage.error() + "Cannot write file " + oldPackagDir
                                            + fileName_
                                            + last_name
                                            + (stg.getKey().contains("header") ? (genhpp_ ? ".hpp" : ".h")
                                                    : ".cxx"));
                                    return false;
                                }
                            }
                        }
                    }
                } else {
                    if (first_obj_) {
                        System.out.println(ColorMessage.error() + "Gen " + packagDir
                            + fileName_ + "PubSubTypes head error");
                    }
                    return false;
                }
                if( first_obj_) {
                    first_obj_ = false;
                }
            }
            first_obj_ = true;
        }
        return true;
    }

    public boolean processExports(Context context, ArrayList<Export> exports, StringTemplate ifcst,
            Map<String, String> extensions, StringTemplateGroup stg, String moduleName) {
        for (Export export : exports) {
            if (export.isIsTypeDeclaration()) {
                TypeDeclaration typedecl = (TypeDeclaration) export;

                // get StringTemplate of the structure
                StringTemplate typest = processTypeDeclaration(context, typedecl, extensions, stg, moduleName, false);

                if (typest != null) {
                    // Add type stringtemplate to interface stringtemplate.
                    ifcst.setAttribute("exports", typest.toString());
                }
            }
        }

        return true;
    }

    public StringTemplate processTypeDeclaration(Context context, TypeDeclaration typedecl,
            Map<String, String> extensions, StringTemplateGroup stg, String moduleName, boolean isCpp) {
        StringTemplate typest = null, extensionst = null;
        String extensionname = null;

        if (typedecl.getTypeCode().getKind() == Kind.KIND_STRUCT) {
            typest = stg.getInstanceOf("struct_type");
            typest.setAttribute("struct", typedecl.getTypeCode());
            if (!moduleName.isEmpty()) {
                context.setNamespaceList(moduleName);
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
            if (isCpp) {
                typest.setAttribute("isfirst", (first_obj_ ? "1" : null));
            }
        }

        return typest;
    }

    private boolean writeFile(String file, StringTemplate template, boolean first_write) {
        boolean returnedValue = false;

        try {
            File handle = new File(file);
            if (!handle.exists() || replace_) {
                FileWriter fw = new FileWriter(file);
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

    private boolean appendTail(String file) {
        boolean returnedValue = false;
        header_first_write_ = false;
        source_first_write_ = false;
        try {
            File handle = new File(file);
            FileWriter fw;
            if (handle.exists()) {
                fw = new FileWriter(file, true);
                fw.write("#endif");
                fw.close();
            }
            returnedValue = true;
        } catch (IOException e) {
            e.printStackTrace();
        }

        return returnedValue;
    }

    private TemplateManager tmanager_ = null;
    private Map<String, StringTemplateGroup> m_params_;
    private String outputDir_ = null;
    private String fileName_ = null;
    private boolean replace_ = false;
    private boolean isCJ_ = false;
    private boolean isCPP_ = false;
    private boolean header_first_write_ = true;
    private boolean source_first_write_ = true;
    private boolean first_obj_ = true;
    private boolean genhpp_ = false;
    private int write_count_ = 0;
    private Context context_ = null;
    private Map<String, Set<String>> idl_member_map_;
}
