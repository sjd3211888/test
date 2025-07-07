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
// 2023-05 ~ 2025-05: Continuous optimization of idlgen and idl2xml: supporting pubsub mode java/jni, RTI idl import and code generation, VBS framework, tostring/get_type_name interface, to_idl_string, -genBoth and more, with macro conflict, static/dynamic type fix, etc.
// ------------------------------------------------------------------

package com.eprosima.vbsrpc.idl.grammar;

import com.eprosima.vbsrpc.idl.tree.*;
import com.eprosima.idl.parser.typecode.TypeCode;
import com.eprosima.idl.parser.tree.TypeDeclaration;
import com.eprosima.vbsdds.idl.parser.typecode.StructTypeCode;
import com.eprosima.idl.parser.tree.Annotation;
import com.eprosima.idl.parser.typecode.Kind;
import com.eprosima.idl.parser.tree.Interface;

import java.util.ArrayList;
import java.util.Stack;
import java.util.Random;

import org.antlr.v4.runtime.Token;

public abstract class Context extends com.eprosima.idl.context.Context
        implements com.eprosima.vbscdr.idl.context.Context {
    public Context(String filename, String file, ArrayList<String> includePaths, boolean clientcode, boolean servercode,
            String appProduct, boolean include_include_prefix) {
        super(filename, file, includePaths);

        m_clientcode = clientcode;
        m_servercode = servercode;
        m_randomGenNames = new Stack<String>();
        m_appProduct = appProduct;
        m_include_include_prefix = include_include_prefix;
    }

    public void setTypelimitation(String lt) {
        m_typelimitation = lt;
    }

    public String getTypelimitation() {
        return m_typelimitation;
    }

    public boolean isClient() {
        return m_clientcode;
    }

    public boolean isServer() {
        return m_servercode;
    }

    public abstract boolean isDds();

    public abstract boolean isRtiTypes();

    /*** Functions inherated from VBSCDR Context ***/

    public boolean isPrintexception() {
        return true;
    }

    public boolean isPrintoperation() {
        return true;
    }

    public String getProduct() {
        return m_appProduct;
    }

    public String getIncludePrefix() {
        if (m_include_include_prefix)
            return m_appProduct + "/";

        return "";
    }

    public String getNamespace() {
        return "rpc";
    }

    public abstract boolean isCdr();

    public abstract boolean isVbscdr();

    public boolean isAnyCdr() {
        return isCdr() || isVbscdr();
    }

    /*** End ***/

    @Override
    public Interface createInterface(String name, Token token) {
        Interface interfaceObject = new Interface(getScopeFile(), isInScopedFile(), getScope(), name, token);
        addInterface(interfaceObject);
        return interfaceObject;
    }

    @Override
    public com.eprosima.vbsrpc.idl.tree.Exception createException(String name, Token token) {
        com.eprosima.vbsrpc.idl.tree.Exception exceptionObject = new com.eprosima.vbsrpc.idl.tree.Exception(
                getScopeFile(), isInScopedFile(), getScope(), name, token);
        addException(exceptionObject);
        return exceptionObject;
    }

    @Override
    public Operation createOperation(String name, Token token) {
        Operation operationObject = new Operation(getScopeFile(), isInScopedFile(), null, name, token);
        return operationObject;
    }

    @Override
    public Param createParam(String name, TypeCode typecode, com.eprosima.idl.parser.tree.Param.Kind kind) {
        Param paramObject = new Param(name, typecode, kind);
        return paramObject;
    }

    @Override
    public boolean isGenerateTypeObject() {
        return false;
    }

    @Override
    public boolean isGenerateTypesC() {
        return false;
    }

    @Override
    public boolean isGenerateTypesROS2() {
        return false;
    }

    @Override
    public String getHeaderGuardName() {
        return getFilename().toUpperCase();
    }

    @Override
    public void addTypeDeclaration(
            TypeDeclaration typedecl) {
        super.addTypeDeclaration(typedecl);

        if (typedecl.getTypeCode().getKind() == Kind.KIND_STRUCT && typedecl.isInScope()) {
            Annotation topicann = typedecl.getAnnotations().get("Topic");

            if (topicann != null && topicann.getValue("value").equalsIgnoreCase("false")) {
                StructTypeCode structtypecode = (StructTypeCode) typedecl.getTypeCode();
                structtypecode.setIsTopic(false);
            } else {
                m_lastStructure = typedecl;
            }
        }
    }

    public void setTypeAliasFlag(boolean type_alias_flag) {
        m_use_type_alias = type_alias_flag;
    }

    public boolean isUseTypeAlias() {
        return m_use_type_alias;
    }

    //// Java block ////
    public void setPackage(
            String pack) {
        if (pack != null && !pack.isEmpty()) {
            m_package = pack + ".";
            m_onlypackage = pack;
            m_packageDir = m_package.replace('.', '/');
        }
    }

    public boolean isIsPackageEmpty() {
        return m_package.isEmpty();
    }

    public String getPackage() {
        return m_package;
    }

    public String getOnlyPackage() {
        return m_onlypackage;
    }

    public String getPackageDir() {
        return m_packageDir;
    }

    public String getPackageUnder() {
        return m_package.replace('.', '_');
    }

    public String getM_lastStructureTopicDataTypeName() {
        String name = new String("");

        if (m_lastStructure != null) {
            if (m_lastStructure.getParent() instanceof Interface) {
                name = name + ((Interface) m_lastStructure.getParent()).getScopedname() + "_" +
                        m_lastStructure.getName();
            } else {
                name = m_lastStructure.getScopedname();
            }
        }
        return name;
    }

    public String getM_lastStructureScopedName() {
        if (m_lastStructure != null) {
            return m_lastStructure.getScopedname();
        }
        return null;
    }

    public String getM_lastStructureScopedNameUnderLine() {
        if (m_lastStructure != null) {
            return m_lastStructure.getCScopedname();
        }
        return null;
    }

    public String getM_lastStructureCScopedName() {
        if (m_lastStructure != null) {
            return m_lastStructure.getCScopedname();
        }
        return null;
    }

    public String getRandomNumber() {
        Random rand = new Random();
        m_lastRandomNum = String.valueOf(Math.abs(rand.nextInt()) + 1);
        return m_lastRandomNum;
    }

    public String getLastRandomNumber() {
        return m_lastRandomNum;
    }

    public boolean isThereIsStructure() {
        if (m_lastStructure != null) {
            return true;
        }
        return false;
    }

    public TypeDeclaration getLastStructure() {
        return m_lastStructure;
    }

    public boolean existsLastStructure() {
        if (m_lastStructure != null) {
            return true;
        }
        return false;
    }

    //// End Java block ////

    public void setSingleLineOutputFlag(boolean singleLineOutput) {
        m_singleLineOutput = singleLineOutput;
    }

    public boolean isSingleLineOutput(){
        return m_singleLineOutput;
    }

    public void setDynamicLengthStringFlag(boolean dynamic_length_string_flag) {
        m_use_dynamic_length_string = dynamic_length_string_flag;
    }

    public boolean isDynamicLengthString() {
        return m_use_dynamic_length_string;
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

    public void setZeroCopyFlag(boolean use_zero_copy) {
        m_use_zero_copy = use_zero_copy;
    }

    public boolean isZeroCopy() {
        return m_use_zero_copy;
    }

    public void setGenSharedLib(boolean gen_shared_lib) {
        m_gen_shared_lib = gen_shared_lib;
    }

    public boolean isGenSharedLib() {
        return m_gen_shared_lib;
    }

    // TODO Para stringtemplate TopicsPlugin de nuestros tipos DDS.
    public String getNewRandomName() {
        String name = "type_" + ++m_randomGenName;
        m_randomGenNames.push(name);
        return name;
    }

    // TODO Para stringtemplate TopicsPlugin de nuestros tipos DDS.
    public String getLastRandomName() {
        return m_randomGenNames.pop();
    }

    public void setLastStructBBMode(boolean last_struct_bb_mode) {
        last_struct_bb_mode_ = last_struct_bb_mode;
    }

    public boolean isLastStructBBMode() {
        return last_struct_bb_mode_;
    }

    public void setRpcFlag(boolean is_rpc) {
        m_is_rpc = is_rpc;
    }

    public boolean isRpc() {
        return m_is_rpc;
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

    public void setDynamicEndPointFlag(boolean is_dynamic_end_point) {
        m_is_dynamic_end_point = is_dynamic_end_point;
    }

    public boolean isDynamicEndPoint() {
        if (!(isUseVbsFramework() && isTestExample() && isGenerateTypeObject())) {
            return false;
        }
        return m_is_dynamic_end_point;
    }

    public void setNotUseRandom(boolean not_use_random) {
        m_not_use_random = not_use_random;
    }

    public boolean isNotUseRandom() {
        return m_not_use_random;
    }

    public void setGenBoth(boolean gen_both) {
        m_gen_both = gen_both;
    }

    public boolean isGenBoth() {
        return m_gen_both;
    }

    private String m_typelimitation = null;

    // TODO Lleva la cuenta de generacion de nuevos nombres.
    private int m_randomGenName = 0;
    private Stack<String> m_randomGenNames = null;
    // TODO Lleva la cuenta del nombre de variables para bucles anidados.
    private char m_loopVarName = 'A';

    // Stores if the user will generate the client source.
    private boolean m_clientcode = true;
    // Stores if the user will generate the server source.
    private boolean m_servercode = true;

    private String m_appProduct = null;
    private boolean m_include_include_prefix = true;
    private boolean m_is_rpc = false;

    //// Java block ////
    // Java package name.
    private String m_package = "";
    private String m_onlypackage = "";
    // Java package dir.
    private String m_packageDir = "";
    //// End Java block

    private TypeDeclaration m_lastStructure = null;
    private String m_fileNameUpper = null;

    private String m_lastRandomNum = "";

    private boolean m_use_type_alias = false;

    private boolean m_singleLineOutput = false;

    private boolean m_use_dynamic_length_string = false;

    private boolean m_is_cj = false;

    private boolean m_use_vbs_framework = false;

    private boolean m_use_zero_copy = false;

    private boolean last_struct_bb_mode_ = false;
    private boolean m_gen_hpp_file = false;
    private boolean m_gen_shared_lib = false;

    private boolean m_multi_to_multi = false;
    private boolean m_use_normal_zero_copy = false;
    private boolean m_is_dynamic_end_point = false;

    private boolean m_gen_both = false;

    private boolean m_not_use_random = false;
}
