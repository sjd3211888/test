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
// 2023-07 ~ 2025-05: Continuous enhancement of idlgen and idl2xml: supporting XML string interface, cpp generation fix, RTI idl import/code generation, Java seq class/unlimited string, tostring/get_type_name, static/dynamic type fix, xcdr, zero copy interface, type validation, vbs idl to rti idl, super large heap, etc.
// ------------------------------------------------------------------

package com.eprosima.idl.context;

import com.eprosima.idl.parser.exception.ParseException;
import com.eprosima.idl.parser.tree.AnnotationDeclaration;
import com.eprosima.idl.parser.tree.AnnotationMember;
import com.eprosima.idl.parser.tree.Definition;
import com.eprosima.idl.parser.tree.Interface;
import com.eprosima.idl.parser.tree.Operation;
import com.eprosima.idl.parser.tree.Param;
import com.eprosima.idl.parser.tree.TypeDeclaration;
import com.eprosima.idl.parser.tree.TreeNode;
import com.eprosima.idl.parser.tree.Module;
import com.eprosima.idl.parser.tree.ConstDeclaration;
import com.eprosima.idl.parser.typecode.BitfieldSpec;
import com.eprosima.idl.parser.typecode.BitsetTypeCode;
import com.eprosima.idl.parser.typecode.BitmaskTypeCode;
import com.eprosima.idl.parser.typecode.EnumMember;
import com.eprosima.idl.parser.typecode.Member;
import com.eprosima.idl.parser.typecode.EnumTypeCode;
import com.eprosima.idl.parser.typecode.Kind;
import com.eprosima.idl.parser.typecode.PrimitiveTypeCode;
import com.eprosima.idl.parser.typecode.StructTypeCode;
import com.eprosima.idl.parser.typecode.TypeCode;
import com.eprosima.idl.parser.typecode.AnyTypeCode;
import com.eprosima.idl.util.Pair;
import com.eprosima.idl.util.Util;
import java.io.File;
import java.io.StringReader;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.Collection;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.Vector;
import java.util.List;
import java.util.LinkedHashSet;
import java.util.LinkedList;
import java.util.Map;
import java.util.NoSuchElementException;
import java.util.Properties;
import java.util.Scanner;
import java.util.Stack;
import org.antlr.v4.runtime.Token;
import java.util.Arrays;
import java.util.Set;
import java.io.FileInputStream;
import java.io.IOException;
import java.security.DigestInputStream;
import java.security.MessageDigest;
import com.eprosima.idl.parser.tree.DefinitionContainer;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class Context
{
    public Context(
            String filename,
            String file,
            ArrayList<String> includePaths)
    {
        // Detect OS
        m_os = System.getProperty("os.name");
        m_userdir = System.getProperty("user.dir");

        m_filename = filename.replace('.', '_');
        m_directoryFile = Util.getIDLFileDirectoryOnly(file);
        m_file = file.startsWith("./") ? file.substring(2) : file;

        // calc Md5 for input IDL file
        int POLYNOMIAL = 0x1021;
        int INITIAL_VALUE = 0xFFFF;

        try {
            FileInputStream fileInputStream = null;
            try {
                fileInputStream = new FileInputStream(file);
                int crc = INITIAL_VALUE;
                byte[] buffer = new byte[1024];
                int bytesRead;
                while ((bytesRead = fileInputStream.read(buffer)) != -1) {
                    for (int i = 0; i < bytesRead; i++) {
                        crc = crc ^ (buffer[i] << 8);
                        for (int j = 0; j < 8; j++) {
                            if ((crc & 0x8000) != 0) {
                                crc = (crc << 1) ^ POLYNOMIAL;
                            } else {
                                crc = crc << 1;
                            }
                        }
                    }
                }
                m_crc16string = Integer.toHexString(crc & 0xFFFF);
            } finally {
                if (fileInputStream != null) {
                    fileInputStream.close();
                }
            }
        } catch (Exception e) {
            e.printStackTrace();
        }

        // Remove absolute directory where the application was executed
        if (startsWith(m_file, m_userdir))
        {
            m_file = m_file.substring(m_userdir.length());

            // Remove possible separator
            if (startsWith(m_file, java.io.File.separator))
            {
                m_file = m_file.substring(1);
            }
        }
        /*
           // Remove relative directory if is equal that where the processed IDL is.
           if(m_directoryFile != null && startsWith(m_file, m_directoryFile))
            m_file = m_file.substring(m_directoryFile.length());
         */

        m_definitions = new ArrayList<Definition>();
        scopedNameMap = new HashMap<>();
        m_modules = new HashMap<String, com.eprosima.idl.parser.tree.Module>();
        m_interfaces = new HashMap<String, Interface>();
        m_exceptions = new HashMap<String, com.eprosima.idl.parser.tree.Exception>();
        m_types = new HashMap<String, TypeDeclaration>();
        m_annotations = new HashMap<String, AnnotationDeclaration>();
        m_keywords = new HashSet<String>();
        m_cppkeywords = new HashSet<String>();
        m_consts = new Vector<ConstDeclaration>();
        m_enums = new Vector<EnumTypeCode>();
        m_allmoduleSet = new HashSet<String>();
        fillKeywords();
        fillCppKeywords();

        // TODO Quitar porque solo es para tipos (previous c) (usado para las excepciones). Mirar alternativa.
        m_includedependency = new HashSet<String>();

        // The scope file has to be initialized because could occur the preprocessor
        // is not called (using -ppDisable).
        m_scopeFile = m_file;

        m_includePaths = new ArrayList<String>();
        m_dependencies = new LinkedHashSet<String>();
        m_directIncludeDependencies = new HashSet<String>();
        m_fileNames = new HashSet<String>();
        m_scopeFilesStack = new Stack<Pair<String, Integer>>();
        for (int i = 0; i < includePaths.size(); ++i)
        {
            String include = (String)includePaths.get(i);
            if (startsWith(include, includeFlag))
            {
                include = include.substring(includeFlag.length());
            }
            if (startsWith(include, m_userdir))
            {
                include = include.substring(m_userdir.length());

                // Remove possible separator
                if (startsWith(include, java.io.File.separator))
                {
                    include = include.substring(1);
                }
            }
            if (m_directoryFile != null && startsWith(include, m_directoryFile))
            {
                include = include.substring(m_directoryFile.length());
            }
            // Add last separator (can be empty by now...)
            if (!include.isEmpty() && include.charAt(include.length() - 1) != java.io.File.separatorChar)
            {
                include += java.io.File.separator;
            }
            m_includePaths.add(include);
        }

        // Reorder include paths;
        int pointer = 0;
        while (pointer < m_includePaths.size())
        {
            int count = pointer + 1;

            while (count < m_includePaths.size())
            {
                if (startsWith(m_includePaths.get(count), m_includePaths.get(pointer)))
                {
                    String first = m_includePaths.get(pointer);
                    String second = m_includePaths.get(count);
                    m_includePaths.set(pointer, second);
                    m_includePaths.set(count, first);
                    break;
                }
                ++count;
            }

            if (count == m_includePaths.size())
            {
                ++pointer;
            }
        }

        // Add here builtin annotations? (IDL 4.2 - 8.3.1 section)
        AnnotationDeclaration idann = createAnnotationDeclaration("id", null);
        idann.addMember(new AnnotationMember("value", new PrimitiveTypeCode(Kind.KIND_LONG), "-1"));

        AnnotationDeclaration autoidann = createAnnotationDeclaration("autoid", null);
        EnumTypeCode autoidannenum = new EnumTypeCode(autoidann.getScopedname(), "autoidannenum", "");
        autoidannenum.addMember(new EnumMember("SEQUENTIAL", ""));
        autoidannenum.addMember(new EnumMember("HASH", ""));
        autoidann.addMember(new AnnotationMember("value", autoidannenum, autoidannenum.getInitialValue()));

        AnnotationDeclaration optionalann = createAnnotationDeclaration("optional", null);
        optionalann.addMember(new AnnotationMember("value", new PrimitiveTypeCode(Kind.KIND_BOOLEAN), "true"));

        AnnotationDeclaration positionann = createAnnotationDeclaration("position", null);
        positionann.addMember(new AnnotationMember("value", new PrimitiveTypeCode(Kind.KIND_USHORT), "-1"));

        AnnotationDeclaration valueann = createAnnotationDeclaration("value", null);
        valueann.addMember(new AnnotationMember("value", new AnyTypeCode(), null));

        AnnotationDeclaration extensibilityann = createAnnotationDeclaration("extensibility", null);
        EnumTypeCode extensibilityannenum = new EnumTypeCode(extensibilityann.getScopedname(), "extensibilityannenum", "");
        extensibilityannenum.addMember(new EnumMember("FINAL", ""));
        extensibilityannenum.addMember(new EnumMember("APPENDABLE", ""));
        extensibilityannenum.addMember(new EnumMember("MUTABLE", ""));
        extensibilityann.addMember(new AnnotationMember("value", extensibilityannenum,
                extensibilityannenum.getInitialValue()));

        createAnnotationDeclaration("final", null);
        createAnnotationDeclaration("appendable", null);
        createAnnotationDeclaration("mutable", null);

        // Create default @Key annotation.
        AnnotationDeclaration keyann = createAnnotationDeclaration("key", null);
        keyann.addMember(new AnnotationMember("value", new PrimitiveTypeCode(Kind.KIND_BOOLEAN), "true"));

        AnnotationDeclaration mustundann = createAnnotationDeclaration("must_understand", null);
        mustundann.addMember(new AnnotationMember("value", new PrimitiveTypeCode(Kind.KIND_BOOLEAN), "true"));

        createAnnotationDeclaration("default_literal", null);

        AnnotationDeclaration rangeann = createAnnotationDeclaration("range", null);
        rangeann.addMember(new AnnotationMember("min", new AnyTypeCode(), null));
        //String.valueOf(Integer.MIN_VALUE)));
        rangeann.addMember(new AnnotationMember("max", new AnyTypeCode(), null));
        //String.valueOf(Integer.MAX_VALUE)));

        AnnotationDeclaration unitsann = createAnnotationDeclaration("units", null);
        unitsann.addMember(new AnnotationMember("value", new PrimitiveTypeCode(Kind.KIND_STRING), ""));

        AnnotationDeclaration defaultann = createAnnotationDeclaration("default", null);
        defaultann.addMember(new AnnotationMember("value", new AnyTypeCode(), null));

        AnnotationDeclaration minann = createAnnotationDeclaration("min", null);
        minann.addMember(new AnnotationMember("value", new AnyTypeCode(), null));

        AnnotationDeclaration maxann = createAnnotationDeclaration("max", null);
        maxann.addMember(new AnnotationMember("value", new AnyTypeCode(), null));

        AnnotationDeclaration bit_boundann = createAnnotationDeclaration("bit_bound", null);
        bit_boundann.addMember(new AnnotationMember("value", new PrimitiveTypeCode(Kind.KIND_USHORT), "-1"));

        AnnotationDeclaration externalann = createAnnotationDeclaration("external", null);
        externalann.addMember(new AnnotationMember("value", new PrimitiveTypeCode(Kind.KIND_BOOLEAN), "true"));

        AnnotationDeclaration nestedann = createAnnotationDeclaration("nested", null);
        nestedann.addMember(new AnnotationMember("value", new PrimitiveTypeCode(Kind.KIND_BOOLEAN), "true"));

        AnnotationDeclaration compatibleann = createAnnotationDeclaration("compatible", null);
        compatibleann.addMember(new AnnotationMember("value", new PrimitiveTypeCode(Kind.KIND_BOOLEAN), "true"));

        AnnotationDeclaration verbatimann = createAnnotationDeclaration("verbatim", null);
        EnumTypeCode verbatimannenum = new EnumTypeCode(verbatimann.getScopedname(), "verbatimannenum", "");
        verbatimannenum.addMember(new EnumMember("BEGIN_FILE", ""));
        verbatimannenum.addMember(new EnumMember("BEFORE_DECLARATION", ""));
        verbatimannenum.addMember(new EnumMember("BEGIN_DECLARATION", ""));
        verbatimannenum.addMember(new EnumMember("END_DECLARATION", ""));
        verbatimannenum.addMember(new EnumMember("AFTER_DECLARATION", ""));
        verbatimannenum.addMember(new EnumMember("END_FILE", ""));
        verbatimann.addMember(new AnnotationMember("language", new PrimitiveTypeCode(Kind.KIND_STRING), "*"));
        // c, c++, java, idl, * (any), or custom value
        verbatimann.addMember(new AnnotationMember("placement", verbatimannenum, "BEFORE_DECLARATION"));
        verbatimann.addMember(new AnnotationMember("text", new PrimitiveTypeCode(Kind.KIND_STRING), ""));

        AnnotationDeclaration serviceann = createAnnotationDeclaration("service", null);
        serviceann.addMember(new AnnotationMember("platform", new PrimitiveTypeCode(Kind.KIND_STRING), "*"));
        // CORBA, DDS, * (any), or custom value

        AnnotationDeclaration onewayann = createAnnotationDeclaration("oneway", null);
        onewayann.addMember(new AnnotationMember("value", new PrimitiveTypeCode(Kind.KIND_BOOLEAN), "true"));

        AnnotationDeclaration amiann = createAnnotationDeclaration("ami", null);
        amiann.addMember(new AnnotationMember("value", new PrimitiveTypeCode(Kind.KIND_BOOLEAN), "true"));

        // Create default @non_serialized annotation.
        AnnotationDeclaration non_serializedann = createAnnotationDeclaration("non_serialized", null);
        non_serializedann.addMember(new AnnotationMember("value", new PrimitiveTypeCode(Kind.KIND_BOOLEAN), "true"));

        //For RPC
        AnnotationDeclaration method = createAnnotationDeclaration("method", null);
        method.addMember(new AnnotationMember("value", new PrimitiveTypeCode(Kind.KIND_BOOLEAN), "true"));

        //For VBSCONFIG
        AnnotationDeclaration vbsconfig = createAnnotationDeclaration("vbsconfig", null);
        StructTypeCode vbsconfigestruct = new StructTypeCode(vbsconfig.getScopedname(), "verbatimannenum", "");
        Member sendtype_config_member = new Member(null, "SendType");
        vbsconfigestruct.addMember(sendtype_config_member);
        Member soa_log_filter_config_member = new Member(null, "SOALogFilter");
        vbsconfigestruct.addMember(soa_log_filter_config_member);
        Member topic_id_config_member = new Member(null, "TopicID");
        vbsconfigestruct.addMember(topic_id_config_member);
        vbsconfig.addMember(new AnnotationMember("vbsconfig_value", vbsconfigestruct, null));
    }

    public String getFilename()
    {
        return m_filename;
    }

    public String getJniFilename()
    {
        return m_filename.replace("_", "_1");
    }

    public String getCrc16string()
    {
        return m_crc16string;
    }

    public void setFilename(
            String filename)
    {
        m_filename = filename;
    }

    public String getTrimfilename()
    {
        return Util.stringTrimAll(m_filename);
    }

    public String getScope()
    {
        return m_scope;
    }

    public void setScope(
            String scope)
    {
        m_scope = scope;
    }

    public void setIdlNamespace(String namespace)
    {
        m_namespace = namespace;
    }

    public String getIdlNamespace()
    {
        return m_namespace;
    }

    /*!
     * @return True if current call is in scoped file.
     */
    public boolean isInScopedFile()
    {
        return (m_scopeFile.startsWith("./") ? m_scopeFile.substring(2) : m_scopeFile).equals(
                    (m_file.startsWith("./") ? m_file.substring(2) : m_file));
    }

    public String getScopeFile()
    {
        return m_scopeFile;
    }

    public boolean isScopeLimitToAll()
    {
        return m_scopeLimitToAll;
    }

    public void setScopeLimitToAll(
            boolean scopeLimitToAll)
    {
        m_scopeLimitToAll = scopeLimitToAll;
    }

    public int getCurrentIncludeLine()
    {
        return m_currentincludeline;
    }

    public Stack<Pair<String, Integer>> getScopeFilesStack()
    {
        return m_scopeFilesStack;
    }

    /*!
     * @brief This function stores a global definition of the IDL file.
     */
    public void addDefinition(Definition definition) {
        String ScopName = "";
        if (definition.isIsModule() || definition.isIsTypeDeclaration()) {
            ScopName = getScopedNameIfPossible(definition);
        }
        List<Definition> existingDefinitions = scopedNameMap.getOrDefault(ScopName, new ArrayList<>());

        if (definition instanceof Module) {
            Module newModule = (Module) definition;
            for (Definition existingDef : existingDefinitions) {
                if (existingDef instanceof Module) {
                    removeDuplicateDefinitions(newModule, (Module) existingDef);
                }
            }
            existingDefinitions.add(newModule.copy());
        } else {
            if (!existingDefinitions.contains(definition)) {
                existingDefinitions.add(definition.copy());
            }
        }

        m_definitions.add(definition.copy());
        scopedNameMap.put(ScopName, existingDefinitions);
    }

    private void removeDuplicateDefinitions(Module newModule, Module existingModule) {
        List<Definition> newDefs = new ArrayList<>(newModule.getDefinitions());
        for (Definition newDef : newDefs) {
            if (newDef instanceof Module) {
                removeDuplicateDefinitions((Module) newDef, existingModule);
            } else {
                if (containsDefinition(existingModule.getDefinitions(), newDef)) {
                    newModule.getDefinitions().remove(newDef);
                }
            }
        }
    }

    private boolean containsDefinition(List<Definition> definitions, Definition definition) {
        for (Definition d : definitions) {
            if (d instanceof Module) {
                if (containsDefinition(((Module)d).getDefinitions(), definition)) {
                    return true;
                }
            }
            if (d.getClass().equals(definition.getClass())) {
                String scopedNameOfD = null;
                if (d instanceof TypeDeclaration) {
                    scopedNameOfD = ((TypeDeclaration) d).getScopedname();
                }
                if (scopedNameOfD != null && scopedNameOfD.equals(getScopedNameIfPossible(definition))) {
                    return true;
                }
            }
        }
        return false;
    }

    private String getScopedNameIfPossible(Definition definition) {
        if (definition instanceof Module) {
            return ((Module) definition).getScopedname();
        } else if (definition instanceof TypeDeclaration) {
            return ((TypeDeclaration) definition).getScopedname();
        }
        return null;
    }

    public ArrayList<Definition> getDefinitions()
    {
        return m_definitions;
    }

    public ArrayList<Definition> getDefinitionsSet()
    {
        Map<String,Definition> definitions_set = new LinkedHashMap<>();
        for (Definition member : getDefinitions()) {
            if (member.isIsTypeDeclaration()) {
                TypeDeclaration typedecl = (TypeDeclaration) member;
                definitions_set.put(typedecl.getScopedname(), member);
            }
            else if (member.isIsModule()) {
                Module container = (Module) member;
                if (definitions_set.containsKey(container.getScopedname())) {
                        Module defCon = (Module)definitions_set.get(container.getScopedname());
                        for (Definition tmp_def : container.getDefinitions()) {
                            String tmp_def_socpe_name = "";
                            if (tmp_def.isIsTypeDeclaration()) {
                                TypeDeclaration tmp_def_typedecl = (TypeDeclaration) tmp_def;
                                tmp_def_socpe_name = tmp_def_typedecl.getScopedname();
                            } else if (tmp_def.isIsModule()) {
                                Module tmp_def_container = (Module) tmp_def;
                                tmp_def_socpe_name = tmp_def_container.getScopedname();
                            }
                            boolean alreadyExists = false;
                            for (Definition existingDef : defCon.getDefinitions()) {
                                if (existingDef.isIsTypeDeclaration()) {
                                    TypeDeclaration existingtypedecl = (TypeDeclaration) existingDef;
                                    if (existingtypedecl.getScopedname().equals(tmp_def_socpe_name)) {
                                        alreadyExists = true;
                                        break;
                                    }
                                } else if (existingDef.isIsModule()) {
                                    Module existing_container = (Module) existingDef;
                                    if (existing_container.getScopedname().equals(tmp_def_socpe_name)) {
                                        alreadyExists = true;
                                        break;
                                    }
                                }
                            }
                            if (!alreadyExists) {
                                defCon.add(tmp_def);
                            }
                        }
                        definitions_set.put(container.getScopedname(), defCon);
                } else {
                    definitions_set.put(container.getScopedname(), member);
                }
            }
        }
        return new ArrayList<>(definitions_set.values());
    }

    /*!
     * @brief This function adds a module to the context.
     * This function is used in the parser.
     */
    public void addModule(
            com.eprosima.idl.parser.tree.Module module)
    {
        if (!m_modules.containsKey(module.getScopedname()))
        {
            m_modules.put(module.getScopedname(), module);
        }
    }

    public com.eprosima.idl.parser.tree.Module existsModule(
            String scopedName)
    {
        if (m_modules.containsKey(scopedName))
        {
            return m_modules.get(scopedName);
        }

        return null;
    }

    public Interface createInterface(
            String name,
            Token token)
    {
        Interface interfaceObject = new Interface(m_scopeFile, isInScopedFile(), m_scope, name, token);
        addInterface(interfaceObject);
        return interfaceObject;
    }

    /*!
     * @brief This function adds a interface to the context.
     * This function is used in the parser.
     */
    protected void addInterface(
            Interface interf)
    {
        Interface prev = m_interfaces.put(interf.getScopedname(), interf);

        // TODO: Excepcion
        if (prev != null)
        {
            System.out.println("Warning: Redefined interface " + prev.getScopedname());
        }
    }

    public Interface getInterface(
            String name)
    {
        int lastIndex = -1;
        Interface returnedValue = m_interfaces.get(name);
        if (returnedValue == null)
        {
            String scope = m_scope;

            while (returnedValue == null && !scope.isEmpty())
            {
                returnedValue = m_interfaces.get(scope + "::" + name);
                lastIndex = scope.lastIndexOf("::");

                if (lastIndex != -1)
                {
                    scope = scope.substring(0, lastIndex);
                }
                else
                {
                    scope = "";
                }
            }
        }

        return returnedValue;
    }

    /*!
     * @brief This function returns all interfaces.
     * This function is used in string templates.
     */
    public ArrayList<Interface> getInterfaces()
    {
        return new ArrayList<Interface>(m_interfaces.values());
    }

    public ArrayList<Interface> getScopedInterfaces()
    {
        ArrayList<Interface> ret = new ArrayList<Interface>();

        for (Interface interf : m_interfaces.values())
        {
            if (interf.isInScope())
            {
                ret.add(interf);
            }
        }

        return ret;
    }

    public ArrayList<Interface> getScopedInterfaces_normal()
    {
        ArrayList<Interface> ret = new ArrayList<Interface>();

        for(Interface interf : m_interfaces.values())
        {
            if(interf.isInScope()) {
                if (!interf.getName().contains("_stream"))
                    ret.add(interf);
            }
        }

        return ret;
    }

    public ArrayList<Interface> getScopedInterfaces_stream()
    {
        ArrayList<Interface> ret = new ArrayList<Interface>();

        for(Interface interf : m_interfaces.values())
        {
            if(interf.isInScope()) {
            if (interf.getName().contains("_stream"))
                    ret.add(interf);
            }
        }

        return ret;
    }

    public com.eprosima.idl.parser.tree.Exception createException(
            String name,
            Token token)
    {
        com.eprosima.idl.parser.tree.Exception exceptionObject = new com.eprosima.idl.parser.tree.Exception(m_scopeFile,
                        isInScopedFile(), m_scope, name, token);
        addException(exceptionObject);
        return exceptionObject;
    }

    /*!
     * @brief This function adds a global exception to the context.
     */
    protected void addException(
            com.eprosima.idl.parser.tree.Exception exception)
    {
        com.eprosima.idl.parser.tree.Exception prev = m_exceptions.put(exception.getScopedname(), exception);

        // TODO: Exception.
        if (prev != null)
        {
            System.out.println("Warning: Redefined exception " + prev.getScopedname());
        }
    }

    /*!
     * @brief This function tries to retrieve a global typecode.
     */
    public com.eprosima.idl.parser.tree.Exception getException(
            String name)
    {
        int lastIndex = -1;
        com.eprosima.idl.parser.tree.Exception returnedValue = m_exceptions.get(name);

        // Probar si no tiene scope, con el scope actual.
        if (returnedValue == null)
        {
            String scope = m_scope;

            while (returnedValue == null && !scope.isEmpty())
            {
                returnedValue = m_exceptions.get(scope + "::" + name);
                lastIndex = scope.lastIndexOf("::");

                if (lastIndex != -1)
                {
                    scope = scope.substring(0, lastIndex);
                }
                else
                {
                    scope = "";
                }
            }
        }

        return returnedValue;
    }

    public Operation createOperation(
            String name,
            Token token)
    {
        Operation operationObject = new Operation(m_scopeFile, isInScopedFile(), null, name, token);
        return operationObject;
    }

    public Param createParam(
            String name,
            TypeCode typecode,
            Param.Kind kind)
    {
        Param paramObject = new Param(name, typecode, kind);
        return paramObject;
    }

    public Param createParam(
            String name,
            Definition definition,
            Param.Kind kind)
    {
        Param paramObject = new Param(name, definition, kind);
        return paramObject;
    }

    public StructTypeCode createStructTypeCode(
            String name)
    {
        StructTypeCode structObject = new StructTypeCode(getExtraNamespace() + (m_scope.isEmpty() || getExtraNamespace().isEmpty() ? "" : "::") + m_scope, name, getExtraNamespace() + (m_namespace.isEmpty() || getExtraNamespace().isEmpty() ? "" : "::") + m_namespace);
        return structObject;
    }

    public BitfieldSpec createBitfieldSpec(
            String size,
            TypeCode type)
    {
        BitfieldSpec object = new BitfieldSpec(getExtraNamespace() + (m_scope.isEmpty() || getExtraNamespace().isEmpty() ? "" : "::") + m_scope, size, type);
        return object;
    }

    public BitsetTypeCode createBitsetTypeCode(
            String name)
    {
        BitsetTypeCode object = new BitsetTypeCode(getExtraNamespace() + (m_scope.isEmpty() || getExtraNamespace().isEmpty() ? "" : "::") + m_scope, name, getExtraNamespace() + (m_namespace.isEmpty() || getExtraNamespace().isEmpty() ? "" : "::") + m_namespace);
        return object;
    }

    public BitmaskTypeCode createBitmaskTypeCode(
            String name)
    {
        BitmaskTypeCode object = new BitmaskTypeCode(getExtraNamespace() + (m_scope.isEmpty() || getExtraNamespace().isEmpty() ? "" : "::") + m_scope, name, getExtraNamespace() + (m_namespace.isEmpty() || getExtraNamespace().isEmpty() ? "" : "::") + m_namespace);
        return object;
    }

    public Collection<TypeDeclaration> getTypes()
    {
        return m_types.values();
    }

    /*!
     * @brief This function adds a global typecode to the context.
     */
    public void addTypeDeclaration(
            TypeDeclaration typedecl)
    {
        TypeDeclaration prev = m_types.put(typedecl.getScopedname(), typedecl);

        m_lastStructure_name=typedecl.getScopedname();
        m_lastStructure_rti_name=typedecl.getCScopedname();
        if (prev != null)
        {
            throw new ParseException(typedecl.getToken(), "was redefined");
        }
    }

    /*!
     * @brief This function returns a global typecode of the context.
     */
    public TypeDeclaration getTypeDeclaration(
            String scopedName)
    {
        return m_types.get(scopedName);
    }

    /*!
     * @brief This function tries to retrieve a global typecode.
     */
    public TypeCode getTypeCode(
            String name)
    {
        int lastIndex = -1;
        TypeCode returnedValue = null;
        TypeDeclaration typedecl = m_types.get(name);
        // Probar si no tiene scope, con el scope actual.
        if (typedecl == null)
        {
            String scope = getExtraNamespace() + (m_scope.isEmpty() || getExtraNamespace().isEmpty() ? "" : "::") + m_scope;

            while (typedecl == null && scope != null && !scope.isEmpty())
            {
                typedecl = m_types.get((scope.equals("") ?  "" : scope + "::") + name);
                lastIndex = scope.lastIndexOf("::");

                if (lastIndex != -1)
                {
                    scope = scope.substring(0, lastIndex);
                }
                else
                {
                    scope = "";
                }
            }
        }

        if (typedecl != null)
        {
            returnedValue = typedecl.getTypeCode();
        }

        return returnedValue;
    }

    public AnnotationDeclaration createAnnotationDeclaration(
            String name,
            Token token)
    {
        AnnotationDeclaration annotationObject = new AnnotationDeclaration(m_scopeFile,
                        isInScopedFile(), getExtraNamespace() + (m_scope.isEmpty() || getExtraNamespace().isEmpty() ? "" : "::") + m_scope, name, token);
        addAnnotationDeclaration(annotationObject);
        return annotationObject;
    }

    /*!
     * @brief This function adds an annotation to the context.
     */
    protected void addAnnotationDeclaration(
            AnnotationDeclaration annotation)
    {
        AnnotationDeclaration prev = m_annotations.put(annotation.getScopedname(), annotation);

        // TODO: Exception.
        if (prev != null)
        {
            System.out.println("Warning: Redefined annotation " + prev.getScopedname());
        }
    }

    public AnnotationDeclaration getAnnotationDeclaration(
            String name)
    {
        int lastIndex = -1;
        AnnotationDeclaration returnedValue = m_annotations.get(name);

        // Probar si no tiene scope, con el scope actual.
        if (returnedValue == null)
        {
            String scope = m_scope;

            while (returnedValue == null && !scope.isEmpty())
            {
                returnedValue = m_annotations.get(scope + "::" + name);
                lastIndex = scope.lastIndexOf("::");

                if (lastIndex != -1)
                {
                    scope = scope.substring(0, lastIndex);
                }
                else
                {
                    scope = "";
                }
            }
        }

        return returnedValue;
    }

    /*!
     * @brief This function add a new library dependency to the project.
     */
    public void addDependency(
            String dependency)
    {
        m_dependencies.add(dependency);
    }

    /*!
     * @brief This function get the library dependencies of a project.
     */
    public LinkedHashSet<String> getDependencies()
    {
        // At this level the dependencies are in reverse order. Return them
        // in correct order.
        LinkedHashSet<String> set = new LinkedHashSet<String>();
        LinkedList<String> list = new LinkedList<String>(m_dependencies);
        Iterator<String> it = list.descendingIterator();

        while (it.hasNext())
        {
            String dep = it.next();

            if (getOS().contains("Windows"))
            {
                // In windows substitute \\ by /
                int count = 0;
                while ((count = dep.indexOf("/")) != -1)
                {
                    dep = dep.substring(0, count) + "\\" + dep.substring(count + 1);
                }
            }
            set.add(dep);
        }

        return set;
    }

    /*!
     * @brief This function is used in the stringtemplates. For these reason this function
     * returns an ArrayList
     */
    public ArrayList<String> getDirectIncludeDependencies()
    {
        return new ArrayList<String>(m_directIncludeDependencies);
    }

    public ArrayList<String> getFileNames()
    {
        return new ArrayList<String>(m_fileNames);
    }

    // TODO Quitar porque solo es para tipos (previous c) (usado para las excepciones). Mirar alternativa.
    /*!
     * @brief This function add a new include dependency to the project.
     * This dependency will be needed to include our generated file with the types that
     * the (previous c) DDS middleware doesn't generate (right now only exceptions).
     * The include dependencies are added without the .idl extension.
     */
    public void addIncludeDependency(
            String dependency)
    {
        // Remove .idl extension.
        String dep = dependency.substring(0, dependency.length() - 4);
        // Remove directory if it is the same than main IDL file.
        if (m_directoryFile != null && startsWith(dep, m_directoryFile))
        {
            dep = dep.substring(m_directoryFile.length());
        }
        m_includedependency.add(dep);
    }

    // TODO Quitar porque solo es para tipos (previous c) (usado para las excepciones). Mirar alternativa.
    /*!
     * @brief This function is used in the stringtemplates. For these reason this function
     * returns an ArrayList
     */
    public ArrayList<String> getIncludeDependencies()
    {
        return new ArrayList<String>(m_includedependency);
    }

    /*!
     * @brief This function is call when a preprocessor line was found by the lexer.
     * In case the line referring to the content included file, this function sets this file as current scope file.
     * Also this function saves the scope file in the library dependecy map.
     * In case it is a #include directive, this is saved as direct include dependency.
     */
    public void processPreprocessorLine(
            String line,
            int nline)
    {
        // If there is a line referring to the content of an included file.
        if (line.startsWith("# "))
        {
            String line_ = line.substring(2);

            /* The received preprocessor line has the following form:
             * ' numline filename flags'
             * where:
             * - numline Number of the line where the include was.
             * - filename The filename whose content was included.
             * - flags
             */
            Scanner scanner = new Scanner(line_);

            // Read numline
            int numline = scanner.nextInt();

            line_ = scanner.nextLine();
            scanner = new Scanner(line_).useDelimiter("\"");

            // Read filename
            scanner.next();
            String file = scanner.next();

            // Read flags.
            boolean systemFile = false, enteringFile = false, exitingFile = false;

            if (m_os.contains("Linux"))
            {
                try
                {
                    line_ = scanner.nextLine();
                    scanner = new Scanner(line_);
                    scanner.next();

                    while (true)
                    {
                        Integer flag = scanner.nextInt();

                        if (flag == 1)
                        {
                            enteringFile = true;
                        }
                        else if (flag == 2)
                        {
                            exitingFile = true;
                        }
                        else if (flag == 3)
                        {
                            systemFile = true;
                        }
                    }
                }
                catch (NoSuchElementException ex)
                {
                    // The line finishes.
                }
            }

            // Only not system files are processed.
            if (!systemFile)
            {
                if (!m_scopeFile.equals(file))
                {
                    // Remove absolute directory where the application was executed
                    if (startsWith(file, m_userdir))
                    {
                        file = file.substring(m_userdir.length());

                        // Remove possible separator
                        if (startsWith(file, java.io.File.separator))
                        {
                            file = file.substring(1);
                        }
                    }
                    // Remove relative ./ directory.
                    if (startsWith(file, currentDirS))
                    {
                        file = file.substring(currentDirS.length());
                        // Remove possible separator
                        if (startsWith(file, java.io.File.separator))
                        {
                            file = file.substring(1);
                        }
                    }

                    //if it is a idl file.
                    if (file.substring(file.length() - 4, file.length()).equals(".idl"))
                    {
                        if (!m_scopeFile.equals(file))
                        {
                            if (!m_scopeFilesStack.empty() && m_scopeFilesStack.peek().first().equals(file))
                            {
                                m_scopeFilesStack.pop();

                                // Add to dependency if there is different IDL file than the processed
                                addDependency(m_scopeFile);

                                // See if it is a direct dependency.
                                if (file.equals(m_file))
                                {
                                    String includeFile = m_scopeFile;
                                    // Remove relative directory if is equal that where the processed IDL is.
                                    if (m_directoryFile != null && startsWith(includeFile, m_directoryFile))
                                    {
                                        includeFile = includeFile.substring(m_directoryFile.length());
                                    }
                                    // Remove relative directory if is equal to a include path.
                                    for (int i = 0; i < m_includePaths.size(); ++i)
                                    {
                                        if (startsWith(includeFile, m_includePaths.get(i)))
                                        {
                                            includeFile = includeFile.substring(m_includePaths.get(i).length());
                                            break;
                                        }
                                    }

                                    m_directIncludeDependencies.add(includeFile.substring(0,
                                            includeFile.length() - 4).replace('.', '_'));
                                    File name_file = new File(includeFile);
                                    String file_name_idl = name_file.getName();

                                    m_fileNames.add(file_name_idl.substring(0,
                                    file_name_idl.length() - 4).replace('.', '_'));
                                }
                            }
                            else
                            {
                                m_scopeFilesStack.push(new Pair<String, Integer>(m_scopeFile,
                                        nline - m_currentincludeline - 1));
                            }

                            m_scopeFile = file;
                        }
                    }
                }

                //Update the current line.
                m_currentincludeline = nline - numline;
            }
        }
    }

    protected String getOS()
    {
        return m_os;
    }

    protected boolean startsWith(
            String st,
            String prefix)
    {
        if (m_os.contains("Windows"))
        {
            return st.toLowerCase().startsWith(prefix.toLowerCase());
        }

        return st.startsWith(prefix);
    }

    /*** Function to generate random loop variables in string templates ***/
    public String getNewLoopVarName()
    {
        m_loopVarName = 'A';
        return Character.toString(m_loopVarName);
    }

    public boolean isCheckNewLetterVarName() {
        m_letterName = 'A';
        Character.toString(m_letterName);
        return true;
    }

    public String getNextLetterVarName()
    {
        return Character.toString(m_letterName++);
    }

    public String getNextLoopVarName()
    {
        return Character.toString(++m_loopVarName);
    }

    public String removeEscapeCharacter(
            String id)
    {
        if (checkKeyword(id))
        {
            //id = "_" + id;
            //return id + " is a keyword, use escape character if you want to use it as identifier (_" + id + ")";
        }
        if (checkCppKeyword(id))
        {
            id = "_cxx_" + id;
        }
        return id;
    }

    public void addConstDecl(ConstDeclaration consts)
    {
        m_consts.add(consts);
    }

    public void addEnumDecl(EnumTypeCode enums)
    {
        m_enums.add(enums);
    }

    public List<ConstDeclaration> getConstDecls()
    {
        return new ArrayList<ConstDeclaration>(m_consts);
    }

    public List<EnumTypeCode> getEnumDecls()
    {
        return new ArrayList<EnumTypeCode>(m_enums);
    }

    public String adjustScope(String name, String initialScope, boolean isFirst)
    {
        int index = 0;
        String tmpScope = initialScope;
        boolean flag = isFirst;
        while ((index = name.indexOf("::", index)) != -1) {
            index += 2;
            if (!flag) {
                flag = true;
                continue;
            }
            if (tmpScope.lastIndexOf("::") != -1) {
                tmpScope = tmpScope.substring(0, tmpScope.lastIndexOf("::"));
            } else {
                tmpScope = "";
                break;
            }
        }
        return tmpScope;
    }

    public ConstDeclaration getConstDecl(String name, String scopName)
    {
        int index = name.indexOf("::");
        String tmp_name_1 = null, tmp_name_2 = null, tmp_name_3 = name;
        String tmp_scop_1 = scopName, tmp_scop_2 = scopName;
        if (index != -1) {
            tmp_scop_1 = adjustScope(name, scopName, true);
            tmp_scop_2 = adjustScope(name, scopName, false);
        }
        tmp_name_1 = tmp_scop_1 == "" ? name : getExtraNamespace() + (tmp_scop_1.isEmpty() || getExtraNamespace().isEmpty() ? "" : "::") + tmp_scop_1 + "::" + name;
        tmp_name_2 = tmp_scop_2 == "" ? name : getExtraNamespace() + (tmp_scop_2.isEmpty() || getExtraNamespace().isEmpty() ? "" : "::") + tmp_scop_2 + "::" + name;

        for (ConstDeclaration cdcl : m_consts)
        {
            if (cdcl.getScopedname().equals(tmp_name_3) || cdcl.getScopedname().equals(tmp_name_1) || cdcl.getScopedname().equals(tmp_name_2))
            {
                return cdcl;
            }
        }
        return null;
    }

    public EnumMember getEnumDecl(String name, String scopName)
    {
        name = getExtraNamespace() + (name.isEmpty() || getExtraNamespace().isEmpty() ? "" : "::") + name;
        int index = name.indexOf("::");
        String tmp_name_1 = null, tmp_name_2 = null, tmp_name_3 = name;
        String tmp_scop_1 = scopName, tmp_scop_2 = scopName;
        if (index != -1) {
            tmp_scop_1 = adjustScope(name, scopName, true);
            tmp_scop_2 = adjustScope(name, scopName, false);
        }
        tmp_name_1 = tmp_scop_1 == "" ? name : getExtraNamespace() + (tmp_scop_1.isEmpty() || getExtraNamespace().isEmpty() ? "" : "::") + tmp_scop_1 + "::" + name;
        tmp_name_2 = tmp_scop_2 == "" ? name : getExtraNamespace() + (tmp_scop_2.isEmpty() || getExtraNamespace().isEmpty() ? "" : "::") + tmp_scop_2 + "::" + name;

        for (EnumTypeCode enumdec : m_enums)
        {
            for (Member enummember : enumdec.getMembers()) {
                if ((enumdec.getScopenormaltail() + enummember.getName()).equals(tmp_name_3) ||
                    (enumdec.getScopenormaltail() + enummember.getName()).equals(tmp_name_1) ||
                    (enumdec.getScopenormaltail() + enummember.getName()).equals(tmp_name_2))
                {
                    return (EnumMember)enummember;
                }
            }

        }
        return null;
    }

    public String getEnumDeclScopname(String name, String scopName)
    {
        int index = name.indexOf("::");
        String tmp_name_1 = null, tmp_name_2 = null, tmp_name_3 = name;
        String tmp_scop_1 = scopName, tmp_scop_2 = scopName;
        if (index != -1) {
            tmp_scop_1 = adjustScope(name, scopName, true);
            tmp_scop_2 = adjustScope(name, scopName, false);
        }
        tmp_name_1 = tmp_scop_1 == "" ? name : getExtraNamespace() + (tmp_scop_1.isEmpty() || getExtraNamespace().isEmpty() ? "" : "::") + tmp_scop_1 + "::" + name;
        tmp_name_2 = tmp_scop_2 == "" ? name : getExtraNamespace() + (tmp_scop_2.isEmpty() || getExtraNamespace().isEmpty() ? "" : "::") + tmp_scop_2 + "::" + name;

        for (EnumTypeCode enumdec : m_enums)
        {
            for (Member enummember : enumdec.getMembers()) {
                if ((enumdec.getScopenormaltail() + enummember.getName()).equals(tmp_name_3) ||
                    (enumdec.getScopenormaltail() + enummember.getName()).equals(tmp_name_1) ||
                    (enumdec.getScopenormaltail() + enummember.getName()).equals(tmp_name_2))
                {
                    return enumdec.getScopenormaltail() + enummember.getName();
                }
            }

        }
        return null;
    }

    public String checkIdentifier(
            Definition.Kind kind,
            String scope,
            String id)
    {
        if (checkKeyword(id))
        {
            id = "_" + id;
            //return id + " is a keyword, use escape character if you want to use it as identifier (_" + id + ")";
        }

        String scopedname = (scope == null || scope.isEmpty()) ? id : scope + "::" + id;

        // Check definitions
        for (Definition def : m_definitions)
        {
            if (def instanceof TreeNode)
            {
                TreeNode tn = (TreeNode)def;
                if (m_ignore_case
                        ? tn.getScopedname().equalsIgnoreCase(scopedname)
                        : tn.getScopedname().equals(scopedname)
                        )
                {
                    boolean error = true;

                    if (kind == Definition.Kind.MODULE && tn instanceof com.eprosima.idl.parser.tree.Module)
                    {
                        error = false;
                    }
                    else if (kind == Definition.Kind.TYPE_DECLARATION &&
                            tn instanceof com.eprosima.idl.parser.tree.TypeDeclaration &&
                            !((com.eprosima.idl.parser.tree.TypeDeclaration)tn).getTypeCode().isDefined())
                    {
                        error = false;
                    }

                    if (error)
                    {
                        return scopedname + " is already defined (Definition: " + def + ")";
                    }
                }
            }
        }

        // Check modules
        for (String type : m_modules.keySet())
        {
            if (m_ignore_case
                    ? type.equalsIgnoreCase(scopedname)
                    : type.equals(scopedname)
                    )
            {
                if (kind != Definition.Kind.MODULE)
                {
                    return scopedname + " is already defined (Module: " + type + ")";
                }
            }
        }

        // Check interfaces
        for (String type : m_interfaces.keySet())
        {
            if (m_ignore_case
                    ? type.equalsIgnoreCase(scopedname)
                    : type.equals(scopedname)
                    )
            {
                return scopedname + " is already defined (Interface: " + type + ")";
            }
        }

        // Check Exceptions
        for (String type : m_exceptions.keySet())
        {
            if (m_ignore_case
                    ? type.equalsIgnoreCase(scopedname)
                    : type.equals(scopedname)
                    )
            {
                return scopedname + " is already defined (Exception: " + type + ")";
            }
        }

        // Check TypeDeclarations
        for (Map.Entry<String, TypeDeclaration> type : m_types.entrySet())
        {
            if (m_ignore_case
                    ? type.getKey().equalsIgnoreCase(scopedname)
                    : type.getKey().equals(scopedname)
                    )
            {
                if (type.getValue().getTypeCode().isDefined())
                {
                    return scopedname + " is already defined (Type: " + type + ")";
                }
            }
        }

        // Check Annotations, only check annotations against other annotations
        if (kind == Definition.Kind.ANNOTATION)
        {
            for (String anno : m_annotations.keySet())
            {
                if (m_ignore_case
                        ? anno.equalsIgnoreCase(scopedname)
                        : anno.equals(scopedname)
                        )
                {
                    return scopedname + " is already defined (Annotation: " + anno + ")";
                }
            }
        }

        if (id.startsWith("_")) // Escaped identifier?
        {
            id = id.substring(1);
        }

        return null;
    }

    public void ignore_case(
            boolean ignore_case)
    {
        m_ignore_case = ignore_case;
    }

    protected void fillKeywords()
    {
        m_keywords.add("setraises");
        m_keywords.add("out");
        m_keywords.add("emits");
        m_keywords.add("string");
        m_keywords.add("switch");
        m_keywords.add("publishes");
        m_keywords.add("typedef");
        m_keywords.add("uses");
        m_keywords.add("primarykey");
        m_keywords.add("custom");
        m_keywords.add("octet");
        m_keywords.add("sequence");
        m_keywords.add("import");
        m_keywords.add("struct");
        m_keywords.add("native");
        m_keywords.add("readonly");
        m_keywords.add("finder");
        m_keywords.add("raises");
        m_keywords.add("void");
        m_keywords.add("private");
        m_keywords.add("eventtype");
        m_keywords.add("wchar");
        m_keywords.add("in");
        m_keywords.add("default");
        m_keywords.add("public");
        m_keywords.add("short");
        m_keywords.add("long");
        m_keywords.add("enum");
        m_keywords.add("wstring");
        m_keywords.add("context");
        m_keywords.add("home");
        m_keywords.add("factory");
        m_keywords.add("exception");
        m_keywords.add("getraises");
        m_keywords.add("const");
        m_keywords.add("ValueBase");
        m_keywords.add("valuetype");
        m_keywords.add("supports");
        m_keywords.add("module");
        m_keywords.add("Object");
        m_keywords.add("truncatable");
        m_keywords.add("unsigned");
        m_keywords.add("fixed");
        m_keywords.add("union");
        m_keywords.add("oneway");
        m_keywords.add("any");
        m_keywords.add("char");
        m_keywords.add("case");
        m_keywords.add("float");
        m_keywords.add("boolean");
        m_keywords.add("multiple");
        m_keywords.add("abstract");
        m_keywords.add("inout");
        m_keywords.add("provides");
        m_keywords.add("consumes");
        m_keywords.add("double");
        m_keywords.add("typeprefix");
        m_keywords.add("typeid");
        m_keywords.add("attribute");
        m_keywords.add("local");
        m_keywords.add("manages");
        m_keywords.add("interface");
        m_keywords.add("component");
        m_keywords.add("set");
        m_keywords.add("map");
        m_keywords.add("bitfield");
        m_keywords.add("bitset");
        m_keywords.add("bitmask");
        m_keywords.add("annotation");
    }

    protected void fillCppKeywords()
    {
        m_cppkeywords.add("asm");
        m_cppkeywords.add("auto");
        m_cppkeywords.add("bool");
        m_cppkeywords.add("break");
        m_cppkeywords.add("case");
        m_cppkeywords.add("catch");
        m_cppkeywords.add("char");
        m_cppkeywords.add("class");
        m_cppkeywords.add("const");
        m_cppkeywords.add("const_cast");
        m_cppkeywords.add("continue");
        m_cppkeywords.add("default");
        m_cppkeywords.add("delete");
        m_cppkeywords.add("do");
        m_cppkeywords.add("double");
        m_cppkeywords.add("dynamic_cast");
        m_cppkeywords.add("else");
        m_cppkeywords.add("enum");
        m_cppkeywords.add("explicit");
        m_cppkeywords.add("export");
        m_cppkeywords.add("extern");
        m_cppkeywords.add("false");
        m_cppkeywords.add("float");
        m_cppkeywords.add("for");
        m_cppkeywords.add("friend");
        m_cppkeywords.add("goto");
        m_cppkeywords.add("if");
        m_cppkeywords.add("inline");
        m_cppkeywords.add("int");
        m_cppkeywords.add("long");
        m_cppkeywords.add("mutable");
        m_cppkeywords.add("namespace");
        m_cppkeywords.add("new");
        m_cppkeywords.add("operator");
        m_cppkeywords.add("private");
        m_cppkeywords.add("protected");
        m_cppkeywords.add("public");
        m_cppkeywords.add("register");
        m_cppkeywords.add("reinterpret_cast");
        m_cppkeywords.add("return");
        m_cppkeywords.add("short");
        m_cppkeywords.add("signed");
        m_cppkeywords.add("sizeof");
        m_cppkeywords.add("static");
        m_cppkeywords.add("static_cast");
        m_cppkeywords.add("switch");
        m_cppkeywords.add("template");
        m_cppkeywords.add("this");
        m_cppkeywords.add("throw");
        m_cppkeywords.add("true");
        m_cppkeywords.add("try");
        m_cppkeywords.add("typedef");
        m_cppkeywords.add("typeid");
        m_cppkeywords.add("typename");
        m_cppkeywords.add("union");
        m_cppkeywords.add("unsigned");
        m_cppkeywords.add("using");
        m_cppkeywords.add("virtual");
        m_cppkeywords.add("void");
        m_cppkeywords.add("volatile");
        m_cppkeywords.add("wchar_t");
        m_cppkeywords.add("while");
        m_cppkeywords.add("override");
        m_cppkeywords.add("final");
        m_cppkeywords.add("transaction_safe");
    }

    protected boolean checkKeyword(
            String id)
    {
        boolean return_value = false;

        for (String keyword : m_keywords)
        {
            if (m_ignore_case
                    ? keyword.equalsIgnoreCase(id)
                    : keyword.equals(id)
                    )
            {
                return_value = true;
                break;
            }
        }

        return return_value;
    }

    protected boolean checkCppKeyword(
            String id)
    {
        boolean return_value = false;

        for (String keyword : m_cppkeywords)
        {
            if (m_ignore_case
                    ? keyword.equalsIgnoreCase(id)
                    : keyword.equals(id)
                    )
            {
                return_value = true;
                break;
            }
        }

        return return_value;
    }

    public String concatStringLiterals(String literal) {
        String regex = "(?<!\\\\)\"([ \r\t\u000C\n]*)\"";
        Pattern pattern = Pattern.compile(regex);
        Matcher matcher = pattern.matcher(literal);

        StringBuffer result = new StringBuffer();
        boolean escapeHex = false;

        while (matcher.find()) {
            matcher.appendReplacement(result, "\"" + matcher.group(1) + "\"");
        }
        matcher.appendTail(result);

        String[] substrings = result.toString().split("(?<=\"|(?<!\\\\)([ \r\t\u000C\n])\")");

        StringBuilder finalResult = new StringBuilder();
        for (String str : substrings) {
            if (escapeHex) {
                str = "\"" + str;
            }
            escapeHex = false;
            if (str.matches("\\\\x[a-fA-F0-9\\s]+$")) {
                escapeHex = true;
            }
            finalResult.append(str).append(escapeHex ? "\"" : "");
        }

        String finalStr = finalResult.toString();
        return finalStr;
    }

    public void setTargetStructScop(String scop) {
        m_targetStructScop = scop;
    }

    public String getTargetStructScop() {
        return m_targetStructScop;
    }

    public void setCJ(boolean is_cj) {
        m_is_cj = is_cj;
    }

    public boolean isCJ() {
        return m_is_cj;
    }

    public void setGenHpp(boolean gen_hpp_file) {
        m_gen_hpp_file = gen_hpp_file;
    }

    public boolean isGenHpp() {
        return m_gen_hpp_file;
    }

    public void setUseVbsFrameworkFlag(boolean use_vbs_framework) {
        m_use_vbs_framework = use_vbs_framework;
    }

    public boolean isUseVbsFramework() {
        return m_use_vbs_framework;
    }

    public boolean isNotUseVbsFramework() {
        return !m_use_vbs_framework;
    }

    public String getTargetStructScopFileName() {
        return m_targetStructScop == "" ? m_filename : m_targetStructScop + "_" + m_filename;
    }

    public void setMaxBufferSize(int max_buffer_size) {
        m_max_buffer_size = max_buffer_size;
    }

    public int getMaxBufferSize() {
        return m_max_buffer_size;
    }

    public void setTestExampleFlag(boolean use_test_example) {
        m_use_test_example = use_test_example;
    }

    public boolean isTestExample() {
        return m_use_test_example;
    }

    public void setTestCrc(boolean use_test_crc) {
        m_use_test_crc = use_test_crc;
    }

    public boolean isTestCrc() {
        return m_use_test_crc;
    }

    public void setOnlyFileName(String only_file_name) {
        m_only_file_name = only_file_name;
    }

    public String getOnlyFileName() {
        return m_only_file_name;
    }

    public void setIsMultiFlag(boolean is_multi_to_multi) {
        m_multi_to_multi = is_multi_to_multi;
    }

    public boolean getIsMultiFlag() {
        return m_multi_to_multi;
    }

    public void setNormalZeroCopyFlag(boolean use_zero_copy) {
        m_use_normal_zero_copy = use_zero_copy;
    }

    public boolean isNormalZeroCopy() {
        return m_use_normal_zero_copy;
    }

    public void setZeroCopyFlag(boolean use_zero_copy) {
        m_use_zero_copy = use_zero_copy;
    }

    public boolean isZeroCopy() {
        return m_use_zero_copy;
    }

    public void setTopStructName(String struct_name) {
        m_topStructList.add(struct_name);
    }

    public boolean checkTopStructName(String struct_name) {
        for (String element : m_topStructList) {
            if (struct_name.equals(element)) {
                return true;
            }
        }
        return false;
    }

    public void setLastStructBBMode(boolean last_struct_bb_mode) {
        last_struct_bb_mode_ = last_struct_bb_mode;
    }

    public boolean isLastStructBBMode() {
        return last_struct_bb_mode_;
    }

    public void setRmPrintHelper(boolean rm_print_helper) {
        m_rm_print_helper = rm_print_helper;
    }

    public boolean isRmPrintHelper() {
        return m_rm_print_helper;
    }

    public final void setNamespaceList(String namespaceString) {
        String[] parts = namespaceString.split("::");
        m_namespaceList = new ArrayList<>(Arrays.asList(parts));
    }

    public List<String> getNamespaceList() {
        return m_namespaceList;
    }

    public final void setAllModuleList(String namespaceString) {
        m_allmoduleSet.add(namespaceString);
    }

    public ArrayList<String> getAllModuleList() {
        ArrayList<String> allmoduleList = new ArrayList<>(m_allmoduleSet);
        return allmoduleList;
    }

    public void setRmGlobalNP(boolean rm_global_np) {
        m_rm_global_np = rm_global_np;
    }

    public boolean isRmGlobalNP() {
        return m_rm_global_np;
    }

    public void setBigAlignment(boolean big_alignment) {
        m_big_alignment = big_alignment;
    }

    public boolean isBigAlignment() {
        return m_big_alignment;
    }

    public void setHasMutable(boolean has_mutable) {
        m_has_mutable = has_mutable;
    }

    public boolean isHasMutable() {
        return m_has_mutable;
    }

    public void setUseUDP(boolean is_use_udp) {
        m_is_use_udp = is_use_udp;
    }

    public boolean isUseUDP() {
        return m_is_use_udp;
    }

    public void setUseDebugLog(boolean is_use_debug_info) {
        m_is_use_debug_info = is_use_debug_info;
    }

    public boolean isUseDebugLog() {
        return m_is_use_debug_info;
    }

    public String getLastStructureName() {
        return m_lastStructure_name;
    }

    public String getLastStructRtiName() {
        return m_lastStructure_rti_name;
    }

    public void setExtraNamespace(String extra_namespace) {
        m_extra_namespace = extra_namespace;
    }

    public void setRtiIdlFlag (boolean rti_idl) {
        m_rti_idl = rti_idl;
    }

    public boolean isRtiIdl() {
        return m_rti_idl;
    }

    public String getExtraNamespace() {
        return m_extra_namespace;
    }

    public void setHeapAllocation(boolean use_heap_allocation_strategy) {
        m_use_heap_allocation_strategy = use_heap_allocation_strategy;
    }

    public boolean isHeapAllocation() {
        return m_use_heap_allocation_strategy;
    }

    public void setPureStructure(boolean pure_structure) {
        m_pure_structure = pure_structure;
    }

    public boolean isPureStructure() {
        return m_pure_structure;
    }

    public void setHideInternals(boolean hide_internals) {
        m_hide_internals = hide_internals;
    }

    public boolean isHideInternals() {
        return m_hide_internals;
    }

    public void setMemoryContact(boolean memory_contact) {
        m_memory_contact = memory_contact;
    }

    public boolean isMemoryContact() {
        return m_memory_contact;
    }

    public void setSizeUpperLimit(int size_upper_limit) {
        m_size_upper_limit = size_upper_limit;
    }

    public int getSizeUpperLimit() {
        return m_size_upper_limit;
    }

    public void setStaticCapacity (int static_seq_capacity) {
        m_static_seq_capacity = static_seq_capacity;
    }

    public boolean isUseStaticCapacity() {
        return m_static_seq_capacity != 0;
    }

    public int getStaticCapacity() {
        return m_static_seq_capacity;
    }

    public void setRpcFlag(boolean is_rpc) {
        m_is_rpc = is_rpc;
    }

    public boolean isRpc() {
        return m_gen_both ? true : m_is_rpc;
    }

    public void setGenBoth(boolean gen_both) {
        m_gen_both = gen_both;
    }

    public boolean isGenBoth() {
        return m_gen_both;
    }

    public void setUsePackOne(boolean use_pack_one) {
        m_use_pack_one = use_pack_one;
    }

    public boolean isUsePackOne() {
       return m_use_pack_one;
    }

    /*** End ***/
    private String m_lastStructure_name = null;
    private String m_lastStructure_rti_name = null;
    // OS
    String m_os = null;
    String m_userdir = null;

    private int m_max_buffer_size = 0;
    private String m_filename = "";
    private String m_file = "";
    private String m_directoryFile = "";
    private String m_crc16string = "";
    private String m_targetStructScop = "";

    private String m_only_file_name = null;

    private List<String> m_topStructList = null;

    private String m_scope = "";
    private String m_namespace = "";
    private String m_scopeFile = "";
    private boolean m_scopeLimitToAll = false;
    private boolean m_is_cj = false;
    private boolean m_use_zero_copy = false;
    private boolean m_use_normal_zero_copy = false;
    private boolean m_use_test_example = false;
    private boolean m_use_test_crc = false;
    private boolean m_multi_to_multi = false;
    private boolean m_rm_print_helper = false;
    private boolean m_rm_global_np = false;
    private boolean m_big_alignment = false;
    private boolean m_has_mutable = false;
    private boolean m_is_use_udp = false;
    private boolean m_is_use_debug_info = false;
    private boolean m_is_dynamic_end_point = false;
    private boolean m_gen_both = false;
    private boolean m_is_rpc = false;

    private int m_currentincludeline = 0;

    final String currentDirS = "." + File.separator;
    final String includeFlag = "-I";

    //! Store all global definitions.
    private ArrayList<Definition> m_definitions;

    private Map<String, List<Definition>> scopedNameMap;

    private List<String> m_namespaceList;
    private HashSet<String> m_allmoduleSet = null;
    //! Map that contains all modules that were found processing the IDL file (after preprocessing):
    private HashMap<String, com.eprosima.idl.parser.tree.Module> m_modules = null;
    //! Map that contains all interfaces that were found processing the IDL file (after preprocessing):
    private HashMap<String, Interface> m_interfaces = null;
    //! Map that contains all global exceptions that were found processing the IDL file (after preprocessing).
    private HashMap<String, com.eprosima.idl.parser.tree.Exception> m_exceptions = null;
    //! Map that contains all types that were found processing the IDL file (after preprocessing).
    protected HashMap<String, TypeDeclaration> m_types = null;
    //! Map that contains all annotations that where found processing the IDL file.
    private HashMap<String, AnnotationDeclaration> m_annotations = null;

    private ArrayList<String> m_includePaths = null;
    //! Set that contains the library dependencies that were found because there was a line of the preprocessor.
    private LinkedHashSet<String> m_dependencies = null;

    //! Set that contains the direct include dependencies in the IDL file. Used to regenerate the IDL in a supported form.
    private HashSet<String> m_directIncludeDependencies = null;

    //! Set that contains the Idl file name.
    private HashSet<String> m_fileNames = null;

    // TODO Quitar porque solo es para tipos (previous c) (usado para las excepciones). Mirar alternativa.
    //! Set that contains the include dependencies that force to include our type generated file (right now only with exceptions in (previous c) DDS types).
    private HashSet<String> m_includedependency = null;

    // TODO Lleva la cuenta del nombre de variables para bucles anidados.
    private char m_loopVarName = 'A';

    private Stack<Pair<String, Integer>> m_scopeFilesStack;

    // All grammar keywords
    private HashSet<String> m_keywords = null;

    // All cpp keywords
    private HashSet<String> m_cppkeywords = null;

    private boolean m_ignore_case = true;

    private Vector<ConstDeclaration> m_consts = null;

    private Vector<EnumTypeCode> m_enums = null;

    private boolean m_use_vbs_framework = false;

    private boolean last_struct_bb_mode_ = false;

    private boolean m_gen_hpp_file = false;

    private boolean m_rti_idl = false;

    private boolean m_use_heap_allocation_strategy = false;

    private boolean m_hide_internals = false;

    private boolean m_pure_structure = false;

    private boolean m_memory_contact = false;

    private int m_size_upper_limit = 0;

    private int m_static_seq_capacity = 0;

    private char m_letterName = 'A';

    private String m_extra_namespace = "";

    private boolean m_use_pack_one = false;
}
