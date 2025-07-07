/*
 * Copyright (c) 2025 Li Auto Inc. and its affiliates
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.eprosima.vbscdr.idl.generator;

import com.eprosima.idl.context.Context;
import com.eprosima.idl.generator.manager.TemplateManager;
import com.eprosima.idl.parser.tree.AnnotationDeclaration;
import com.eprosima.idl.parser.tree.ConstDeclaration;
import com.eprosima.idl.parser.tree.Definition;
import com.eprosima.idl.parser.tree.Export;
import com.eprosima.idl.parser.tree.Interface;
import com.eprosima.idl.parser.tree.TypeDeclaration;
import com.eprosima.idl.parser.typecode.MemberedTypeCode;
import com.eprosima.idl.parser.typecode.Member;
import com.eprosima.idl.parser.typecode.Kind;
import com.eprosima.idl.parser.typecode.TypeCode;
import com.eprosima.log.ColorMessage;
import com.eprosima.vbsdds.util.ValueHolder;
import java.io.*;
import java.util.ArrayList;
import java.util.Map;
import java.util.HashMap;
import org.antlr.stringtemplate.StringTemplate;
import org.antlr.stringtemplate.StringTemplateGroup;
import java.util.Set;
import java.util.HashSet;
import java.nio.file.Files;
public class IDLTypesGenerator {
    public IDLTypesGenerator(TemplateManager tmanager, String outputDir, boolean replace) {
        tmanager_ = tmanager;
        outputDir_ = outputDir;
        replace_ = replace;
        idl_member_set_ = new HashSet<>();
        seenNames = new HashSet<>();
        stg_ = tmanager_.createStringTemplateGroup("IdlType");
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
        String fileName_ = determineFileName(context);
        if (!checkAndWriteInitialTag(packagDir + fileName_)) {
            System.out.println(ColorMessage.error() + "File " + packagDir
                    + fileName_ + " exists, please check");
            return false;
        }

        boolean returnedValue = processDefinitions(context, definitions, packagDir, "", extensions);

        return returnedValue;
    }

    private boolean writeMainTypeDeclarationFile(
            StringTemplate typest,
            Context context,
            String moduleName,
            String fileName,
            String packageDir,
            Map<String, String> extensions
    ) {
        StringTemplate st = stg_.getInstanceOf("main");
        st.setAttribute("ctx", context);
        st.setAttribute("definitions", typest.toString());
        st.setAttribute("module_name", !moduleName.isEmpty() ? moduleName : null);

        if (extensions != null) {
            String extensionName = extensions.get("main");
            if (extensionName != null) {
                StringTemplate extST = stg_.getInstanceOf(extensionName);
                extST.setAttribute("ctx", context);
                st.setAttribute("extension", extST.toString());
            }
        }

        if (!writeFile(packageDir + fileName, st)) {
            System.out.println(ColorMessage.error() + "Cannot write file " + packageDir + fileName);
            return false;
        }
        return true;
    }

    private void writeAnnotationTemplate(
            Context context,
            AnnotationDeclaration annotation,
            Map<String, String> extensions
    ) {
        StringTemplate annotationST = stg_.getInstanceOf("annotation");
        annotationST.setAttribute("ctx", context);
        annotationST.setAttribute("annotation", annotation);

        if (extensions != null) {
            String extensionName = extensions.get("annotation");
            if (extensionName != null) {
                StringTemplate extST = stg_.getInstanceOf(extensionName);
                extST.setAttribute("ctx", context);
                extST.setAttribute("annotation", annotation);
                annotationST.setAttribute("extension", extST.toString());
            }
        }
    }

    private boolean writeConstDeclarationFile(
            Context context,
            ConstDeclaration constDecl,
            String fileName,
            String packageDir
    ) {
        StringTemplate constST = stg_.getInstanceOf("const_decl");
        constST.setAttribute("ctx", context);
        constST.setAttribute("const", constDecl);

        return writeFile(packageDir + fileName, constST);
    }

    public boolean processDefinitions(Context context, ArrayList<Definition> definitions, String packagDir,
            String moduleName, Map<String, String> extensions) {
        String fileName = determineFileName(context);
        if (definitions != null) {
            for (Definition definition : definitions) {
                if (definition.isIsModule()) {
                    com.eprosima.idl.parser.tree.Module module = (com.eprosima.idl.parser.tree.Module) definition;

                    if (!processDefinitions(context, module.getDefinitions(), packagDir,
                            (moduleName.isEmpty() ? "" : moduleName + "::") + module.getName(), extensions))
                        return false;
                } else if (definition.isIsInterface()) {
                    System.out.println(ColorMessage.error() + "Not support Interface");
                } else if (definition.isIsTypeDeclaration()) {
                    TypeDeclaration typeDecl = (TypeDeclaration) definition;
                    if (idl_member_set_.contains(typeDecl.getScopedname())) {
                        continue;
                    }
                    idl_member_set_.add(typeDecl.getScopedname());

                    if (!processMemberTypecode(context, typeDecl, extensions, moduleName, packagDir)) {
                        return false;
                    }

                    StringTemplate typeST = processTypeDeclaration(context, typeDecl, extensions, moduleName);
                    if (typeST != null) {
                        if (!writeMainTypeDeclarationFile(typeST, context, moduleName, fileName, packagDir, extensions)) {
                            return false;
                        }
                    }
                } else if (definition.isIsAnnotation()) {
                    AnnotationDeclaration annotation = (AnnotationDeclaration) definition;
                    writeAnnotationTemplate(context, annotation, extensions);
                } else if (definition.isIsConstDeclaration()) {
                    // const_decl(ctx, parent, const)
                    ConstDeclaration const_decl = (ConstDeclaration) definition;
                    if (!seenNames.add(const_decl.getCScopedname())) {
                        continue;
                    }
                    if (!writeConstDeclarationFile(context, const_decl, fileName, packagDir)) {
                        System.out.println(ColorMessage.error() + "Cannot write file " + packagDir + fileName );
                        return false;
                    }
                }
            }
        }

        return true;
    }

    public StringTemplate processTypeDeclaration(Context context, TypeDeclaration typedecl,
        Map<String, String> extensions, String moduleName) {
        StringTemplate typest = null, extensionst = null;
        String extensionname = null;

        // 将kind和template key建立映射关系
        Map<Integer, String[]> kindTemplateMap = new HashMap<>();
        kindTemplateMap.put(Kind.KIND_STRUCT, new String[]{"struct_type", "struct"});
        kindTemplateMap.put(Kind.KIND_UNION, new String[]{"union_type", "union"});
        kindTemplateMap.put(Kind.KIND_ENUM, new String[]{"enum_type", "enum"});
        kindTemplateMap.put(Kind.KIND_BITSET, new String[]{"bitset_type", "bitset"});
        kindTemplateMap.put(Kind.KIND_BITMASK, new String[]{"bitmask_type", "bitmask"});
        kindTemplateMap.put(Kind.KIND_ALIAS, new String[]{"typedef_decl", "typedefs"});

        int kind = typedecl.getTypeCode().getKind();
        String[] templateInfo = kindTemplateMap.get(kind);

        if (templateInfo != null) {
            String templateKey = templateInfo[0];
            String attrName = templateInfo[1];

            typest = stg_.getInstanceOf(templateKey);
            typest.setAttribute(attrName, typedecl.getTypeCode());

            // Get extension
            if (extensions != null && (extensionname = extensions.get(templateKey)) != null) {
                extensionst = stg_.getInstanceOf(extensionname);
                extensionst.setAttribute(attrName, typedecl.getTypeCode());
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

            int index = typedecl.getTypeCode().getNameOrigin().lastIndexOf("::");
            String type_socp_normal = index == -1 ? "" : typedecl.getTypeCode().getNameOrigin().substring(0, index);
            if (!type_socp_normal.isEmpty() && moduleName.isEmpty()) {
                typest.setAttribute("module_name", type_socp_normal.startsWith("::") ? type_socp_normal.substring(2) : type_socp_normal);
                return typest;
            }

            typest.setAttribute("module_name",
                    (!type_socp_normal.isEmpty()
                            ? (!moduleName.isEmpty() ? (moduleName + "::") : "") + (type_socp_normal.startsWith("::") ? type_socp_normal.substring(2) : type_socp_normal)
                            : (!moduleName.isEmpty() ? moduleName : null)));
        }

        return typest;
    }

    public StringTemplate processMemberTypecodeByMember(Context context, TypeCode member,
            Map<String, String> extensions, String moduleName) {
        StringTemplate typest = null, extensionst = null;
        String extensionname = null;

        if (member.getKind() == Kind.KIND_STRUCT) {
            typest = stg_.getInstanceOf("struct_type");
            typest.setAttribute("struct", member);

            // Get extension
            if (extensions != null && (extensionname = extensions.get("struct_type")) != null) {
                extensionst = stg_.getInstanceOf(extensionname);
                extensionst.setAttribute("struct", member);
            }
        } else if (member.getKind() == Kind.KIND_UNION) {
            typest = stg_.getInstanceOf("union_type");
            typest.setAttribute("union", member);

            // Get extension
            if (extensions != null && (extensionname = extensions.get("union_type")) != null) {
                extensionst = stg_.getInstanceOf(extensionname);
                extensionst.setAttribute("union", member);
            }
        } else if (member.getKind() == Kind.KIND_ENUM) {
            typest = stg_.getInstanceOf("enum_type");
            typest.setAttribute("enum", member);

            // Get extension
            if (extensions != null && (extensionname = extensions.get("enum_type")) != null) {
                extensionst = stg_.getInstanceOf(extensionname);
                extensionst.setAttribute("enum", member);
            }
        } else if (member.getKind() == Kind.KIND_BITSET) {
            typest = stg_.getInstanceOf("bitset_type");
            typest.setAttribute("bitset", member);

            // Get extension
            if (extensions != null && (extensionname = extensions.get("bitset_type")) != null) {
                extensionst = stg_.getInstanceOf(extensionname);
                extensionst.setAttribute("bitset", member);
            }
        } else if (member.getKind() == Kind.KIND_BITMASK) {
            typest = stg_.getInstanceOf("bitmask_type");
            typest.setAttribute("bitmask", member);

            // Get extension
            if (extensions != null && (extensionname = extensions.get("bitmask_type")) != null) {
                extensionst = stg_.getInstanceOf(extensionname);
                extensionst.setAttribute("bitmask", member);
            }
        } else if (member.getKind() == Kind.KIND_ALIAS) {
            typest = stg_.getInstanceOf("typedef_decl");
            typest.setAttribute("typedefs", member);

            // Get extension
            if (extensions != null && (extensionname = extensions.get("typedef_decl")) != null) {
                extensionst = stg_.getInstanceOf(extensionname);
                extensionst.setAttribute("typedefs", member);
            }
        }

        if (typest != null) {
            // Generate extension
            if (extensionst != null) {
                extensionst.setAttribute("ctx", context);
                extensionst.setAttribute("parent", null);
                typest.setAttribute("extension", extensionst.toString());
            }

            // Main stringtemplate
            typest.setAttribute("ctx", context);
            typest.setAttribute("parent", null);
            int index = member.getNameOrigin().lastIndexOf("::");
            String type_socp_normal = index == -1 ? "" : member.getNameOrigin().substring(0, index);

            if (!type_socp_normal.isEmpty() && moduleName.isEmpty()) {
                typest.setAttribute("module_name", type_socp_normal.startsWith("::") ? type_socp_normal.substring(2) : type_socp_normal);
                return typest;
            }

            typest.setAttribute("module_name", (!type_socp_normal.isEmpty() ?
                            (!moduleName.isEmpty() ? (moduleName + "::") : "") + (type_socp_normal.startsWith("::") ? type_socp_normal.substring(2) :type_socp_normal)
                            : (!moduleName.isEmpty() ? moduleName : null)));
        }

        return typest;
    }

    public boolean processMemberTypecode(Context context, TypeDeclaration typedecl,
            Map<String, String> extensions, String moduleName, String packagDir) {
        String fileName_ = determineFileName(context);
        if (typedecl.getTypeCode().getKind() == Kind.KIND_STRUCT ||
            typedecl.getTypeCode().getKind() == Kind.KIND_UNION ||
            typedecl.getTypeCode().getKind() == Kind.KIND_ENUM ||
            typedecl.getTypeCode().getKind() == Kind.KIND_BITSET ||
            typedecl.getTypeCode().getKind() == Kind.KIND_BITMASK) {
            MemberedTypeCode tmp_member_type = (MemberedTypeCode)typedecl.getTypeCode();

            for (TypeCode tmp_member: tmp_member_type.getAllInnerMembers()) {
                if (tmp_member == null) {
                    continue;
                }
                if (tmp_member.getKind() == Kind.KIND_STRUCT ||
                    tmp_member.getKind() == Kind.KIND_UNION ||
                    tmp_member.getKind() == Kind.KIND_ENUM ||
                    tmp_member.getKind() == Kind.KIND_BITSET ||
                    tmp_member.getKind() == Kind.KIND_BITMASK) {
                    if (idl_member_set_.contains(tmp_member.getScopednamenormal())) {
                        continue;
                    } else {
                        idl_member_set_.add(tmp_member.getScopednamenormal());
                    }
                    StringTemplate typest = processMemberTypecodeByMember(context, tmp_member, extensions, moduleName);
                    if (typest != null) {
                        // Save file.
                        StringTemplate st = stg_.getInstanceOf("main");
                        st.setAttribute("ctx", context);
                        st.setAttribute("definitions", typest.toString());
                        int index = tmp_member.getNameOrigin().lastIndexOf("::");
                        String type_socp_normal = index == -1 ? "" : tmp_member.getNameOrigin().substring(0, index);
                        if (!type_socp_normal.isEmpty() && moduleName.isEmpty()) {
                            typest.setAttribute("module_name", type_socp_normal.startsWith("::") ? type_socp_normal.substring(2) : type_socp_normal);
                        } else {
                            typest.setAttribute("module_name", (!type_socp_normal.isEmpty() ?
                                        (!moduleName.isEmpty() ? (moduleName + "::") : "") + (type_socp_normal.startsWith("::") ? type_socp_normal.substring(2) :type_socp_normal)
                                        : (!moduleName.isEmpty() ? moduleName : null)));
                        }
                        StringTemplate extensionst = null;
                        String extensionname = null;
                        if (extensions != null && (extensionname = extensions.get("main")) != null) {
                            extensionst = stg_.getInstanceOf(extensionname);
                            extensionst.setAttribute("ctx", context);
                            st.setAttribute("extension", extensionst.toString());
                        }
                        if (!writeFile(packagDir + fileName_, st)) {
                            System.out.println(ColorMessage.error() + "Cannot write file " + packagDir
                                    + fileName_);
                            return false;
                        }
                    }
                }
            }
        }
        return true;
    }

    private boolean writeFile(String file, StringTemplate template) {
        boolean returnedValue = false;

        try {
            File handle = new File(file);
            boolean fileExists = handle.exists();
            try (FileWriter writer = new FileWriter(file, fileExists)) {
                writer.write(template.toString());
            }
            returnedValue = true;
        } catch (IOException e) {
            e.printStackTrace();
        }

        return returnedValue;
    }

    private boolean checkAndWriteInitialTag(String filePath) {
        boolean initialized = false;

        try {
            File fileHandle = new File(filePath);
            if (!fileHandle.exists() || replace_) {
                try (FileWriter writer = new FileWriter(filePath)) {}
                initialized = true;
            } else {
                System.err.println("Error: " + filePath + " exists. Skipping.");
            }
        } catch (IOException e) {
            e.printStackTrace();
        }

        return initialized;
    }

    private String determineFileName(Context context) {
        if (context.isTestExample()) {
            return context.getLastStructureName() + "_RTI.idl";
        } else {
            return context.getFilename() + "_RTI.idl";
        }
    }

    private TemplateManager tmanager_ = null;
    private StringTemplateGroup stg_ = null;
    private String outputDir_ = null;
    private boolean replace_ = false;
    private Set<String> idl_member_set_;
    private HashSet<String> seenNames = null;

}
