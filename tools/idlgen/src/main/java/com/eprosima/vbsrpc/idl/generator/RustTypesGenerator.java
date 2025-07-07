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

package com.eprosima.vbsrpc.idl.generator;

import com.eprosima.vbsrpc.idl.grammar.Context;

import com.eprosima.idl.generator.manager.TemplateManager;
import com.eprosima.idl.parser.tree.*;
import com.eprosima.idl.parser.typecode.Kind;
import com.eprosima.idl.parser.typecode.TypeCode;
import com.eprosima.log.ColorMessage;
import org.antlr.stringtemplate.StringTemplate;
import org.antlr.stringtemplate.StringTemplateGroup;

import java.io.*;
import java.util.*;
import java.nio.file.Files;
import java.nio.file.StandardCopyOption;

public class RustTypesGenerator {
    public RustTypesGenerator(TemplateManager tmanager, String outputDir, boolean replace, boolean useVbs,
            String fileName) {
        tmanager_ = tmanager;
        outputDir_ = outputDir;
        replace_ = replace;
        fileName_ = fileName;
        idl_member_map_ = new HashMap<>();
        idl_member_map_.put("header", new HashSet<>());
        idl_member_map_.put("source", new HashSet<>());

        m_params_ = new HashMap<>();
        m_params_.put("", tmanager_.createStringTemplateGroup("TypesRustInterface"));
    }

    public boolean generate(Context context, String packagDir, String packag, String libraryName,
            Map<String, String> extensions) {
        ArrayList<Definition> definitions = context.getDefinitions();
        context_ = context;
        boolean returnedValue = processDefinitions(context, definitions, packagDir, packag, extensions, "", packagDir);
        if (returnedValue) {
            returnedValue &= populateModFiles(context, new File(packagDir), replace_, true);
        }
        return returnedValue;
    }

    public boolean processDefinitions(Context context, ArrayList<Definition> definitions, String packagDir,
            String packag, Map<String, String> extensions, String moduleName, String oldPackagDir) {
        if (definitions != null) {
            for (Definition definition : definitions) {
                String uniqueDefId = "";
                if (definition.isIsModule()) {
                    com.eprosima.idl.parser.tree.Module module = (com.eprosima.idl.parser.tree.Module) definition;
                    String outputDir = packagDir + module.getName();
                    File dir = new File(outputDir);
                    if (!dir.exists()) {
                        if (!dir.mkdir()) {
                            System.out.println(
                                    ColorMessage.error() + "Cannot create directory for module " + module.getName());
                            return false;
                        }
                    }

                    String modFilePath = outputDir + File.separator + "mod.rs";
                    File modFile = new File(modFilePath);
                    if (!modFile.exists()) {
                        try {
                            if (!modFile.createNewFile()) {
                                System.out.println(
                                        ColorMessage.error() + "Cannot create mod.rs for module " + module.getName());
                                return false;
                            }
                        } catch (IOException e) {
                            System.out.println(
                                    ColorMessage.error() + "Error creating mod.rs " + modFilePath + ": "
                                            + e.getMessage());
                            return false;
                        }
                    }
                    if (!packagDir.endsWith(File.separator)) {
                        packagDir += File.separator;
                    }
                    if (!processDefinitions(context, module.getDefinitions(), outputDir + File.separator,
                            packag + "." + module.getName(), extensions,
                            (moduleName.isEmpty() ? "" : moduleName + "::") + module.getName(), oldPackagDir))
                        return false;
                } else if (definition.isIsInterface()) {
                    Interface ifc = (Interface) definition;
                    for (Map.Entry<String, StringTemplateGroup> entry : m_params_.entrySet()) {
                        String key = entry.getKey();
                        StringTemplateGroup group = entry.getValue();

                        uniqueDefId = moduleName + ifc.getName();
                        if (!processedDefinitions.add(uniqueDefId)) {
                            continue;
                        }

                        StringTemplate ifcst = group.getInstanceOf("interface");
                        ifcst.setAttribute("ctx", context);
                        ifcst.setAttribute("parent", ifc.getParent());
                        ifcst.setAttribute("interface", ifc);
                        ifcst.setAttribute("module_name", !moduleName.isEmpty() ? moduleName : null);

                        StringTemplate extensionst = null;
                        if (extensions != null) {
                            String extensionName = extensions.get("interface");
                            if (extensionName != null) {
                                extensionst = group.getInstanceOf(extensionName);
                                extensionst.setAttribute("ctx", context);
                                extensionst.setAttribute("parent", ifc.getParent());
                                extensionst.setAttribute("interface", ifc);
                                ifcst.setAttribute("extension", extensionst.toString());
                            }
                        }

                        if (!processExports(context, ifc.getExports(), ifcst, extensions, group, moduleName)) {
                            return false;
                        }

                        StringTemplate st = group.getInstanceOf("main");
                        st.setAttribute("ctx", context);
                        st.setAttribute("definitions", ifcst.toString());
                        st.setAttribute("package", !packag.isEmpty() ? packag : null);
                        st.setAttribute("module_name", !moduleName.isEmpty() ? moduleName : null);

                        if (key.contains("header") || key.contains("source")) {
                            st.setAttribute("interfacename", ifc.getName());
                        }

                        if (extensions != null) {
                            String extensionName = extensions.get("main");
                            if (extensionName != null) {
                                extensionst = group.getInstanceOf(extensionName);
                                extensionst.setAttribute("ctx", context);
                                st.setAttribute("extension", extensionst.toString());
                            }
                        }

                        String last_name = "";
                        int lastUnderscoreIndex = key.lastIndexOf('_');
                        if (lastUnderscoreIndex != -1) {
                            last_name = key.substring(lastUnderscoreIndex);
                        } else {
                            System.out.println("No underscore found in the string.");
                        }

                        String filePath = packagDir + ifc.getName() + ".rs";
                        if (!writeFile(filePath, st)) {
                            System.out.println(ColorMessage.error() + "Cannot write file " + filePath);
                            return false;
                        }
                    }
                } else if (definition.isIsTypeDeclaration()) {
                    for (Map.Entry<String, StringTemplateGroup> stg : m_params_.entrySet()) {
                        TypeDeclaration typedecl = (TypeDeclaration) definition;
                        uniqueDefId = typedecl.getCScopedname();
                        if (processedDefinitions.contains(uniqueDefId)) {
                            continue;
                        } else {
                            processedDefinitions.add(uniqueDefId);
                        }
                        StringTemplate typest = processTypeDeclaration(context, typedecl, extensions,
                                stg.getValue(), moduleName);

                        if (typest != null) {
                            StringTemplate st = stg.getValue().getInstanceOf("main");
                            st.setAttribute("ctx", context);
                            st.setAttribute("definitions", typest.toString());
                            st.setAttribute("package", (!packag.isEmpty() ? packag.split("\\.")[0] : null));

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

                            if (!writeFile(packagDir + typedecl.getName() + ".rs", st)) {
                                System.out.println(ColorMessage.error() + "Cannot write file " + packagDir
                                        + typedecl.getName() + ".rs");
                                return false;
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

        Map<Integer, String[]> kindTemplateMap = new HashMap<>();
        kindTemplateMap.put(Kind.KIND_STRUCT,   new String[]{"struct_type", "struct"});
        kindTemplateMap.put(Kind.KIND_UNION,    new String[]{"union_type", "union"});
        kindTemplateMap.put(Kind.KIND_ENUM,     new String[]{"enum_type", "enum"});
        kindTemplateMap.put(Kind.KIND_BITSET,   new String[]{"bitset_type", "bitset"});
        kindTemplateMap.put(Kind.KIND_BITMASK,  new String[]{"bitmask_type", "bitmask"});
        kindTemplateMap.put(Kind.KIND_ALIAS,    new String[]{"typedef_decl", "typedefs"});

        int kind = typedecl.getTypeCode().getKind();
        String[] templateInfo = kindTemplateMap.get(kind);

        if (templateInfo != null) {
            String templateKey = templateInfo[0];
            String attrName = templateInfo[1];

            typest = stg.getInstanceOf(templateKey);
            typest.setAttribute(attrName, typedecl.getTypeCode());

            if (extensions != null && (extensionname = extensions.get(templateKey)) != null) {
                extensionst = stg.getInstanceOf(extensionname);
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
        }

        return typest;
    }

    private boolean writeFile(String file, StringTemplate template) {
        File handle = new File(file);
        if (!replace_ && handle.exists()) {
            System.out.println("INFO: " + file + " exists. Skipping.");
            return true;
        }

        try {
            boolean firstTimeForThisFile = !writtenFiles.contains(file);

            if (replace_ && firstTimeForThisFile && handle.exists()) {
                if (!handle.delete()) {
                    System.out.println("Error: Cannot delete existing file " + file);
                    return false;
                }
            }
            boolean append;
            if (replace_) {
                append = !firstTimeForThisFile;
            } else {
                append = false;
            }

            try (FileWriter fw = new FileWriter(file, append)) {
                String data = template.toString();
                fw.write(data, 0, data.length());
            }
            writtenFiles.add(file);
            return true;
        } catch (IOException e) {
            e.printStackTrace();
            return false;
        }
    }

    private boolean prependToFile(String filePath) {
        File file = new File(filePath);
        if (!file.exists() || !file.isFile()) {
            System.out.println("Error: The provided file does not exist or is not a file." + filePath);
            return false;
        }
        File tempFile = new File(file.getAbsolutePath() + ".tmp");

        try (BufferedReader reader = new BufferedReader(new FileReader(file));
                BufferedWriter writer = new BufferedWriter(new FileWriter(tempFile))) {

            writer.write("#![allow(non_camel_case_types)]\n\n");
            writer.write("use std::ffi::{c_void, CString, CStr, c_char};\n\n");
            writer.write("use std::os::raw::c_char;\n\n");
            writer.write("use vbs::{TopicData, RpcServer,RpcMessageType};\n\n");
            writer.write("include!(concat!(env!(\"OUT_DIR\"), \"/typesbindings.rs\"));\n\n");

            String line;
            while ((line = reader.readLine()) != null) {
                writer.write(line);
                writer.newLine();
            }
        } catch (IOException e) {
            System.out.println(ColorMessage.error() + "Error processing the file: " + e.getMessage());
            return false;
        }

        if (!file.delete()) {
            System.out.println(ColorMessage.error() + "Could not delete original file");
            return false;
        }

        if (!tempFile.renameTo(file)) {
            System.out.println(ColorMessage.error() + "Could not rename temporary file to original file name");
            return false;
        }

        File parentDir = file.getParentFile();
        if (parentDir != null && parentDir.exists() && parentDir.isDirectory()) {
            Stack<File> stack = new Stack<>();
            stack.push(parentDir);

            while (!stack.isEmpty()) {
                File currentDir = stack.pop();
                File[] files = currentDir.listFiles();

                if (files != null) {
                    boolean isEmpty = true;
                    for (File subFile : files) {
                        if (subFile.isDirectory()) {
                            stack.push(subFile);
                        } else {
                            isEmpty = false;
                        }
                    }

                    if (isEmpty) {
                        if (!currentDir.delete()) {
                            System.out.println(ColorMessage.error() + "Cannot delete empty directory "
                                    + currentDir.getAbsolutePath());
                        }
                    }
                }
            }
        }

        return true;
    }

    public boolean populateModFiles(Context context, File directory, boolean replace_, boolean isRoot) {
        if (!directory.isDirectory()) {
            return false;
        }

        File[] files = directory.listFiles();
        if (files == null) {
            return false;
        }

        File modFile = new File(directory, "mod.rs");

        try (FileWriter writer = new FileWriter(modFile, !replace_)) {
            if (isRoot) {
                writer.write("#![allow(non_camel_case_types)]\n\n");
                writer.write("use std::ffi::{c_void, CString, CStr, c_char};\n\n");
                writer.write("use std::collections::BTreeMap;\n\n");
                writer.write("use vbs::{TopicData, RpcServer, RpcMessageType};\n\n");
                if (context.isTestExample()) {
                    writer.write("use serde::{Serialize, Deserialize};\n\n");
                }
                writer.write("include!(concat!(env!(\"OUT_DIR\"), \"/typesbindings.rs\"));\n\n");
            } else {
                writer.write("use super::*;\n\n");
            }

            Set<String> declaredModules = new HashSet<>();

            for (File file : files) {
                String moduleName;
                if (file.isDirectory()) {
                    moduleName = file.getName();
                    if (!declaredModules.contains(moduleName)) {
                        writer.write("pub mod " + moduleName + ";\n");
                        declaredModules.add(moduleName);
                    }
                    if (!populateModFiles(context, file, replace_, false)) {
                        return false;
                    }
                } else if (file.getName().endsWith(".rs") && !file.getName().equals("mod.rs")) {
                    moduleName = file.getName().substring(0, file.getName().length() - 3);
                    if (!declaredModules.contains(moduleName)) {
                        writer.write("include!(\"" + moduleName + ".rs\");\n");
                        declaredModules.add(moduleName);
                    }
                }
            }
        } catch (IOException e) {
            System.out.println("Error writing to mod.rs in " + directory.getPath() + ": " + e.getMessage());
            return false;
        }
        return true;
    }

    public boolean deleteEmptyModuleFolders(File directory) {
        boolean deleted = false;

        if (directory.isDirectory()) {
            File[] files = directory.listFiles();

            if (files != null) {
                for (File file : files) {
                    if (file.isDirectory()) {
                        if (deleteEmptyModuleFolders(file)) {
                            deleted = true;
                        }
                    }
                }
            }
            files = directory.listFiles();
            if (files != null && files.length == 1 && files[0].getName().equals("mod.rs")) {
                if (!files[0].delete()) {
                    System.out.println("Deleted file: " + files[0].getPath() + "fail...");
                }
                if (!directory.delete()) {
                    System.out.println("Deleted empty module folder: " + directory.getPath() + "fail...");
                } else {
                    deleted = true;
                }
            }
        }

        return deleted;
    }

    private TemplateManager tmanager_ = null;
    private Map<String, StringTemplateGroup> m_params_;
    private String outputDir_ = null;
    private static boolean isFirstWrite = true;
    private String fileName_ = null;
    private boolean replace_ = false;
    private Context context_ = null;
    private Map<String, Set<String>> idl_member_map_;

    private static Set<String> writtenFiles = new HashSet<>();
    private static Set<String> processedDefinitions = new HashSet<>();
}
