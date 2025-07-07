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
// 2023-05 ~ 2025-05: Implemented multi-language pubsub/java/jni, DDS namespace injection and macro packaging, zero-copy; fixed macro conflicts, resource/compatibility related bugs.
// ------------------------------------------------------------------

package com.eprosima.vbsdds.idl.grammar;

import com.eprosima.vbsdds.idl.parser.typecode.StructTypeCode;
import com.eprosima.idl.parser.tree.Annotation;
import com.eprosima.idl.parser.tree.Interface;
import com.eprosima.idl.parser.tree.TypeDeclaration;
import com.eprosima.idl.parser.typecode.Kind;
import com.eprosima.idl.parser.typecode.TypeCode;
import com.eprosima.idl.parser.typecode.Member;
import com.eprosima.idl.parser.typecode.MapTypeCode;
import com.eprosima.idl.parser.typecode.MemberedTypeCode;
import com.eprosima.idl.parser.typecode.SequenceTypeCode;
import com.eprosima.idl.parser.typecode.EnumTypeCode;
import java.util.ArrayList;
import java.util.Collection;
import java.util.Map.Entry;
import java.util.AbstractMap.SimpleEntry;
import java.util.Stack;
import java.util.List;
import java.util.Random;

public class Context extends com.eprosima.idl.context.Context implements com.eprosima.vbscdr.idl.context.Context {
    // TODO Remove middleware parameter. It is temporal while cdr and rest don't
    // have async functions.
    public Context(
            String filename,
            String file,
            ArrayList<String> includePaths,
            boolean subscribercode,
            boolean publishercode,
            String appProduct,
            boolean generate_type_object,
            boolean generate_typesc,
            boolean generate_type_ros2) {
        super(filename, file, includePaths);
        m_fileNameUpper = filename.toUpperCase();
        m_subscribercode = subscribercode;
        m_publishercode = publishercode;
        m_randomGenNames = new Stack<String>();

        // m_protocol = protocol;
        // m_ddstypes = ddstypes;

        m_type_object = generate_type_object;
        m_typesc = generate_typesc;
        m_type_ros2 = generate_type_ros2;
        m_topStructList = new ArrayList<String>();
    }

    public void setTypelimitation(
            String lt) {
        m_typelimitation = lt;
    }

    public String getTypelimitation() {
        return m_typelimitation;
    }

    @Override
    public StructTypeCode createStructTypeCode(
            String name) {
        return new StructTypeCode(getExtraNamespace() + (getScope().isEmpty() || getExtraNamespace().isEmpty()  ? "" : "::") + getScope(), name, getExtraNamespace() + (getIdlNamespace().isEmpty() || getExtraNamespace().isEmpty() ? "" : "::") + getIdlNamespace());
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

    public boolean isClient() {
        return m_subscribercode;
    }

    public boolean isServer() {
        return m_publishercode;
    }

    // TODO For stringtemplate TopicsPlugin of our DDS types.
    public String getNewRandomName() {
        String name = "type_" + ++m_randomGenName;
        m_randomGenNames.push(name);
        return name;
    }

    public String getNewLoopVarName() {
        m_loopVarName = 'a';
        return Character.toString(m_loopVarName);
    }

    public String getNextLoopVarName() {
        return Character.toString(++m_loopVarName);
    }

    // TODO For stringtemplate TopicsPlugin of our DDS types.
    public String getLastRandomName() {
        return m_randomGenNames.pop();
    }

    public ArrayList<Entry<String, TypeCode>> getTypeCodesToDefine() {
        ArrayList<Entry<String, TypeCode>> typecodes = new ArrayList<Entry<String, TypeCode>>();

        for (TypeDeclaration type : m_types.values()) {
            if (type.getTypeCode() instanceof MemberedTypeCode && !(type.getTypeCode() instanceof EnumTypeCode)) {
                for (Member member : ((MemberedTypeCode) type.getTypeCode()).getMembers()) {
                    if (member.getTypecode().getKind() == Kind.KIND_SEQUENCE) {
                        getSequencesToDefine(typecodes, (SequenceTypeCode) member.getTypecode());
                    } else if (member.getTypecode().getKind() == Kind.KIND_MAP) {
                        MapTypeCode map = (MapTypeCode) member.getTypecode();
                        if (map.getKeyTypeCode().getKind() == Kind.KIND_SEQUENCE) {
                            getSequencesToDefine(typecodes, (SequenceTypeCode) map.getKeyTypeCode());
                        }
                        if (map.getValueTypeCode().getKind() == Kind.KIND_SEQUENCE) {
                            getSequencesToDefine(typecodes, (SequenceTypeCode) map.getValueTypeCode());
                        }
                    }
                }
            }
        }

        return typecodes;
    }

    private void getSequencesToDefine(
            ArrayList<Entry<String, TypeCode>> typecodes,
            SequenceTypeCode sequence) {
        // Search
        for (Entry<String, TypeCode> entry : typecodes) {
            if (entry.getKey().equals(sequence.getCppTypename())) {
                return;
            }
        }

        TypeCode content = sequence.getContentTypeCode();

        if (content.getKind() == Kind.KIND_SEQUENCE) {
            getSequencesToDefine(typecodes, (SequenceTypeCode) content);
        } else if (content.getKind() == Kind.KIND_MAP) {
            MapTypeCode map = (MapTypeCode) content;
            if (map.getKeyTypeCode().getKind() == Kind.KIND_SEQUENCE) {
                getSequencesToDefine(typecodes, (SequenceTypeCode) map.getKeyTypeCode());
            }
            if (map.getValueTypeCode().getKind() == Kind.KIND_SEQUENCE) {
                getSequencesToDefine(typecodes, (SequenceTypeCode) map.getValueTypeCode());
            }
        }

        typecodes.add(new SimpleEntry<String, TypeCode>(sequence.getCppTypename(), sequence));
    }

    /*** Functions inherited from VBSCDR Context ***/

    @Override
    public boolean isPrintexception() {
        return false;
    }

    @Override
    public boolean isPrintoperation() {
        return false;
    }

    public String getProduct() {
        return "vbscdr";
    }

    public String getNamespace() {
        return "vbscdr";
    }

    public boolean isCdr() {
        return true;
    }

    public boolean isVbscdr() {
        return activateFusion_;
    }

    public boolean isAnyCdr() {
        return true;
    }

    /*** End ***/

    public void setActivateFusion(
            boolean value) {
        activateFusion_ = value;
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

    //// End Java block ////

    public void setTypeAliasFlag(boolean type_alias_flag) {
        m_use_type_alias = type_alias_flag;
    }

    public boolean isUseTypeAlias() {
        return m_use_type_alias;
    }

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

    public void setGenSharedLib(boolean gen_shared_lib) {
        m_gen_shared_lib = gen_shared_lib;
    }

    public boolean isGenSharedLib() {
        return m_gen_shared_lib;
    }

    public void setRmPrintHelper(boolean rm_print_helper) {
        m_rm_print_helper = rm_print_helper;
    }

    public boolean isRmPrintHelper() {
        return m_rm_print_helper;
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

    public void setUseEstimateSize(boolean use_estimate_size) {
        m_use_estimate_size = use_estimate_size;
    }

    public boolean isUseEstimateSize() {
        return m_use_estimate_size;
    }

    private String m_typelimitation = null;

    private List<String> m_topStructList = null;

    // TODO Counts generation of new names.
    private int m_randomGenName = 0;
    private Stack<String> m_randomGenNames = null;
    // TODO Keeps track of variable name for nested loops.
    private char m_loopVarName = 'a';

    // Stores if the user will generate the client source.
    private boolean m_subscribercode = true;
    // Stores if the user will generate the server source.
    private boolean m_publishercode = true;

    private String m_only_file_name = null;

    private TypeDeclaration m_lastStructure = null;

    private boolean m_type_object = false;

    private boolean m_typesc = false;

    private boolean m_type_ros2 = false;

    private boolean m_use_type_alias = false;

    private boolean m_multi_to_multi = false;

    private boolean m_use_dynamic_length_string = false;

    private boolean m_is_cj = false;

    private boolean m_use_vbs_framework = false;

    private boolean m_use_zero_copy = false;

    private boolean m_use_normal_zero_copy = false;

    private boolean m_use_test_example = false;

    private int m_max_buffer_size = 0;

    private boolean m_gen_shared_lib = false;

    private boolean m_rm_print_helper = false;

    private boolean m_rm_global_np = false;

    private boolean m_big_alignment = false;

    private boolean m_use_estimate_size = false;

    private boolean m_singleLineOutput = false;

    @Override
    public boolean isGenerateTypeObject() {
        return m_type_object;
    }

    @Override
    public boolean isGenerateTypesC() {
        return m_typesc;
    }

    @Override
    public boolean isGenerateTypesROS2() {
        return m_type_ros2;
    }

    public String getHeaderGuardName() {
        if (m_lastStructure != null) {
            if (m_lastStructure.getHasScope()) {
                return m_lastStructure.getScope().replaceAll("::", "_").toUpperCase() +
                        "_" + m_fileNameUpper.replaceAll("\\.", "_");
            }
        }
        return m_fileNameUpper;
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

    public void setLastStructBBMode(boolean last_struct_bb_mode) {
        last_struct_bb_mode_ = last_struct_bb_mode;
    }

    public boolean isLastStructBBMode() {
        return last_struct_bb_mode_;
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

    private String m_fileNameUpper = null;

    public void setFilename(
            String filename) {
        super.setFilename(filename);
        m_fileNameUpper = filename.toUpperCase();
    }

    public String getFileNameUpper() {
        return m_fileNameUpper;
    }

    public String getJniFilename() {
        return getFilename().replace("_", "_1");
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

    private String m_lastRandomNum = "";
    //// Java block ////
    // Java package name.
    private String m_package = "";
    private String m_onlypackage = "";
    // Java package dir.
    private String m_packageDir = "";
    private boolean activateFusion_ = false;
    private boolean last_struct_bb_mode_ = false;
    private boolean m_is_dynamic_end_point = false;
    private boolean m_not_use_random = false;
    //// End Java block

}
