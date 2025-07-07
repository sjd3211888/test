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
// 2023-05 ~ 2025-05: Continuous enhancement of idlgen and idl2xml: support for idl2xml parser, RTI idl import and code generation, rpc build optimization, tostring and get_type_name interfaces, xcdr support, type validation, removal of ContainerPrintHelpers, data_to_json support, and @autoid compilation improvement, etc. 
// ------------------------------------------------------------------

package com.eprosima.idl.parser.typecode;

import java.util.ArrayList;
import java.util.List;
import java.util.LinkedHashMap;
import java.util.Map;
import java.util.HashMap;
import java.util.LinkedHashSet;
import java.util.Set;
import org.antlr.stringtemplate.StringTemplate;
import com.eprosima.idl.parser.exception.RuntimeGenerationException;

public class UnionTypeCode extends MemberedTypeCode
{
    public UnionTypeCode(String scope, String name, String namespace)
    {
        super(Kind.KIND_UNION, scope, name, namespace);
        m_discriminatorTypeCode = null;
    }

    public UnionTypeCode(String scope, String name, TypeCode discriminatorTypeCode, String namespace)
    {
        super(Kind.KIND_UNION, scope, name, namespace);
        m_discriminatorTypeCode = discriminatorTypeCode;
    }

    public void setDiscriminatorType(TypeCode discriminatorTypeCode)
    {
        m_discriminatorTypeCode = discriminatorTypeCode;
    }

    @Override
    public String getTypeIdentifier()
    {
        return "0xF1U";
    }

    @Override
    public boolean isObjectType() { return true; }

    @Override
    public boolean isIsUnionType() {return true; }

    /*!
     * @return 0 is ok, -1 the member is repeated, -2 is another default member.
     */
    public int addMember(UnionMember member)
    {
        if(member.isDefault())
        {
            if(m_defaultindex == -1)
                m_defaultindex = getMembers().size();
            else
                return -2;
        }

        // Generate labels
        List<String> internal_labels = member.getInternalLabels();
        List<String> labels = null;
        List<String> values = null;
        List<String> javalabels = null;

        if (m_discriminatorTypeCode.getKind() == Kind.KIND_ALIAS) {
            AliasTypeCode alias_type = (AliasTypeCode) m_discriminatorTypeCode;
            TypeCode trueType = alias_type.getTrueContentTypeCode();
            labels = new ArrayList<String>();
            values = new ArrayList<String>();
            javalabels = new ArrayList<String>();

            for (int count = 0; count < internal_labels.size(); ++count) {
                String label = internal_labels.get(count);
                if (trueType instanceof EnumTypeCode) {
                    EnumTypeCode enum_type = (EnumTypeCode) trueType;
                    labels.add((enum_type.getScope() != null ? enum_type.getScopednamenormal() + "::"
                            : enum_type.getName()) + label);
                    values.add(enum_type.getValueByName(label));
                    javalabels.add(javapackage + enum_type.getJavaScopedname() + "." + label);
                } else {
                    labels.add(label);
                    values.add(label);
                    javalabels.add(label);
                }
            }
        } else if(m_discriminatorTypeCode.getKind() == Kind.KIND_ENUM)
        {
            EnumTypeCode enum_type = (EnumTypeCode)m_discriminatorTypeCode;
            labels = new ArrayList<String>();
            values = new ArrayList<String>();
            javalabels = new ArrayList<String>();

            for(int count = 0; count < internal_labels.size(); ++count)
            {
                labels.add((enum_type.getScope() != null ? enum_type.getScopednamenormal() + "::" : enum_type.getName()) + internal_labels.get(count));
                values.add(enum_type.getValueByName(internal_labels.get(count)));
                javalabels.add(javapackage + enum_type.getJavaScopedname() + "." + internal_labels.get(count));
            }
        }
        else
        {
            labels = internal_labels;
            values = internal_labels;
            javalabels = internal_labels;
        }

        member.setLabels(labels);
        member.setJavaLabels(javalabels);
        for (int i = 0; i < values.size(); i++) {
            String value = values.get(i);
            if (value.isEmpty()) {
                values.set(i, String.valueOf(mt_mpValue));
                mt_mpValue++;
            } else {
                try {
                    int number = 0;
                    if (value.startsWith("0x") || value.startsWith("0X")) {
                        number = Integer.parseInt(value.substring(2), 16);
                        values.set(i, String.valueOf(number));
                    } else if (value.length() == 1) {
                        char singleChar = value.charAt(0);
                        if (!Character.isDigit(singleChar)) {
                            number = singleChar;
                        } else {
                            number = Integer.parseInt(value);
                        }
                        values.set(i, String.valueOf(number));
                        number = singleChar;
                    } else if (value.length() == 3 && value.startsWith("'") && value.endsWith("'")) {
                        char singleChar = value.charAt(1);
                        number = singleChar;
                    } else {
                        number = Integer.parseInt(value);
                    }

                    mt_mpValue = number + 1;
                } catch (NumberFormatException e) {
                    System.out.println("Error: " + value + "is not number");
                }
            }
        }
        member.setValues(values);

        calculate_member_id_(member);

        if(!addMember((Member)member))
            return -1;

        return 0;
    }

    public Member getDefaultMember()
    {
        if(m_defaultindex != -1)
            return getMembers().get(m_defaultindex);

        return null;
    }

    public List<Member> getAllLikeStructMembers() {
        Set<Member> allMembersSet = new LinkedHashSet<>();
        for (Member member : super.getMembers()) {
            addMemberAndInnerMembers(member, allMembersSet);
        }
        return new ArrayList<>(allMembersSet);
    }

    private void addMemberAndInnerMembers(Member member, Set<Member> allMembersSet) {
        if (member.getIsStringed()) {
            return;
        }

        if (member.getTypecode().isIsUnionType() && ((UnionTypeCode)member.getTypecode()).getDiscriminator() != null) {
            Member discriminatorMember = new Member(((UnionTypeCode)member.getTypecode()).getDiscriminator(), "");
            discriminatorMember.setFlagName(getCScopedname() + String.valueOf(getNextAlias(aliasCounter)));
            addMemberAndInnerMembers(discriminatorMember, allMembersSet);
        }

        if (member.getTypecode().isIsSequenceType() || member.getTypecode().isIsType_f()) {
            Member discriminatorMember = new Member(((ContainerTypeCode)member.getTypecode()).getContentTypeCode(), "");
            discriminatorMember.setFlagName(getCScopedname() + String.valueOf(getNextAlias(aliasCounter)));
            addMemberAndInnerMembers(discriminatorMember, allMembersSet);
        }
        if (member.getTypecode().isIsMapType()) {
            Member discriminatorMember_key = new Member(((ContainerTypeCode)member.getTypecode()).getKeyTypeCode(), "");
            discriminatorMember_key.setFlagName( getCScopedname() + String.valueOf(getNextAlias(aliasCounter)));
            addMemberAndInnerMembers(discriminatorMember_key, allMembersSet);
            Member discriminatorMember_value = new Member(((ContainerTypeCode)member.getTypecode()).getValueTypeCode(), "");
            discriminatorMember_value.setFlagName(getCScopedname() + String.valueOf(getNextAlias(aliasCounter)));
            addMemberAndInnerMembers(discriminatorMember_value, allMembersSet);
        }

        if (member.getTypecode().isIsType_a() || member.getTypecode().isIsUnionType()) {
            for (Member innerMember : member.getTypecode().getAllLikeStructMembers()) {
                addMemberAndInnerMembers(innerMember, allMembersSet);
            }
        }

        if (member.getTypecode().isIsType_a() || member.getTypecode().isIsType_c() ||
                member.getTypecode().isIsBitmaskType() || member.getTypecode().isIsBitsetType() ||
                member.getTypecode().isIsUnionType()) {
            member.setIsStringed();
            allMembersSet.add(member);
        }
    }

    @Override
    public String getCppTypename()
    {
        StringTemplate st = getCppTypenameFromStringTemplate();
        st.setAttribute("name", getScopedname());
        return st.toString();
    }

    @Override
    public String getCTypename()
    {
        StringTemplate st = getCTypenameFromStringTemplate();
        st.setAttribute("name", getCScopedname());
        return st.toString();
    }

    @Override
    public String getCwrapperTypename()
    {
        return getCTypename() + "Wrapper";
    }

    @Override
    public String getJavaTypename()
    {
        StringTemplate st = getJavaTypenameFromStringTemplate();
        st.setAttribute("name", getJavaScopedname());
        return st.toString();
    }

    @Override
    public String getRustTypename()
    {
        StringTemplate st = getRustTypenameFromStringTemplate();
        st.setAttribute("name", getRustScopedname());
        return st.toString();
    }

    @Override
    public String getIdlTypename()
    {
        StringTemplate st = getIdlTypenameFromStringTemplate();
        st.setAttribute("name", getScopednamenormal());
        return st.toString();
    }

    @Override
    public String getXmlTypename()
    {
        StringTemplate st = getXmlTypenameFromStringTemplate();
        st.setAttribute("name", getScopedname());
        return st.toString();
    }

    public void setDefaultvalue(String value)
    {
        m_defaultValue = value;
    }

    public void setJavaDefaultvalue(String value)
    {
        m_javaDefaultValue = value;
    }

    // Used in stringtemplates
    public String getDefaultvalue()
    {
        return m_defaultValue;
    }

    @Override
    public String getInitialValue()
    {
        if(getMembers().size() > 0)
        {
            return (getScope() != null ? getScopedname() + "::" : getName()) + getMembers().get(0).getName();
        }

        return "";
    }

    // Used in stringtemplates
    public String getJavaDefaultvalue()
    {
        return m_javaDefaultValue;
    }

    // Used in stringtemplates
    public TypeCode getDiscriminator()
    {
        return m_discriminatorTypeCode;
    }

    // Used in stringtemplates
    public List<String> getTotallabels()
    {
        List<String> returnList = new ArrayList<String>();
        List<Member> mlist = getMembers();
        List<String> labels = null;

        for(int count = 0; count < mlist.size(); ++count)
        {
            if(count != m_defaultindex)
            {
                labels = ((UnionMember)mlist.get(count)).getLabels();
                for(int i = 0; i < labels.size(); ++i)
                    returnList.add(labels.get(i));
            }
        }

        return returnList;
    }

    /*public Pair<Integer, Integer> getMaxSerializedSize(int currentSize, int lastDataAligned)
    {
        List<Member> members = getMembers();
        int lcurrentSize = currentSize, lmaxSize = 0;
        int llastDataAligned = 0;

        Pair<Integer, Integer> dpair = m_discriminatorTypeCode.getMaxSerializedSize(lcurrentSize, lastDataAligned);
        lcurrentSize = dpair.first();

        for(int count = 0; count < members.size(); ++count)
        {
            Pair<Integer, Integer> pair = members.get(count).getTypecode().getMaxSerializedSize(lcurrentSize, dpair.second());

            if(pair.first() > lmaxSize)
            {
                lmaxSize = pair.first();
                llastDataAligned = pair.second();
            }
        }

        return new Pair<Integer, Integer>(lmaxSize, llastDataAligned);
    }

    public int getMaxSerializedSizeWithoutAlignment(int currentSize)
    {
        List<Member> members = getMembers();
        int lcurrentSize = currentSize, lmaxSize = 0;

        lcurrentSize = m_discriminatorTypeCode.getMaxSerializedSizeWithoutAlignment(lcurrentSize);

        for(int count = 0; count < members.size(); ++count)
        {
            int aux = members.get(count).getTypecode().getMaxSerializedSizeWithoutAlignment(lcurrentSize);

            if(aux > lmaxSize)
            {
                lmaxSize = aux;
            }
        }

        return lmaxSize;
    }

    public String getMaxSerializedSize()
    {
        Pair<Integer, Integer> pair = getMaxSerializedSize(0, 0);
        return pair.first().toString();
    }

    public String getMaxSerializedSizeWithoutAlignment()
    {
        return Integer.toString(getMaxSerializedSizeWithoutAlignment(0));
    }*/

    @Override
    public boolean isIsPlain()
    {
        return false;
    }

    private static int getNextAlias(int current) {
        aliasCounter = (current + 1);
        return (current + 1);
    }

    public boolean isHasSeqMember() {
        for (Member element : getMembers()) {
            if (element.getTypecode() == null) {
                continue;
            }
            TypeCode tmp_type_code = element.getTypecode();
            if(tmp_type_code != null && tmp_type_code.isIsSequenceType() &&
                !(ctx.isZeroCopy() && !((SequenceTypeCode)tmp_type_code).isUnbound())) {
                return true;
            }
            while(tmp_type_code instanceof ContainerTypeCode) {
                tmp_type_code = ((ContainerTypeCode)tmp_type_code).getTrueContentTypeCode();
                if(tmp_type_code != null && tmp_type_code.isIsSequenceType() &&
                    !(ctx.isZeroCopy() && !((SequenceTypeCode)tmp_type_code).isUnbound())) {
                    return true;
                }
            }
        }
        return false;
    }

    public boolean isHasArrayMember() {
        for (Member element : getMembers()) {
            if (element.getTypecode() == null) {
                continue;
            }
            TypeCode tmp_type_code = element.getTypecode();
            if(tmp_type_code != null && (tmp_type_code.isIsArrayType() ||
                (tmp_type_code.isIsSequenceType() && ctx.isZeroCopy() && !((SequenceTypeCode)tmp_type_code).isUnbound()))) {
                return true;
            }
            while(tmp_type_code instanceof ContainerTypeCode) {
                tmp_type_code = ((ContainerTypeCode)tmp_type_code).getTrueContentTypeCode();
                if(tmp_type_code != null && (tmp_type_code.isIsArrayType() ||
                    (tmp_type_code.isIsSequenceType() && ctx.isZeroCopy() && !((SequenceTypeCode)tmp_type_code).isUnbound()))) {
                    return true;
                }
            }
        }
        return false;
    }

    public boolean isHasMapMember() {
        for (Member element : getMembers()) {
            if (element.getTypecode() == null) {
                continue;
            }
            TypeCode tmp_type_code = element.getTypecode();
            if(tmp_type_code != null && tmp_type_code.isIsMapType()) {
                return true;
            }
            while(tmp_type_code instanceof ContainerTypeCode) {
                tmp_type_code = ((ContainerTypeCode)tmp_type_code).getTrueContentTypeCode();
                if(tmp_type_code != null && tmp_type_code.isIsMapType()) {
                    return true;
                }
            }
        }
        return false;
    }

    public boolean isHasWstringMember() {
        for (Member element : getMembers()) {
            if (element.getTypecode() == null) {
                continue;
            }
            TypeCode tmp_type_code = element.getTypecode();
            if(tmp_type_code != null && tmp_type_code.isIsWStringType()) {
                return true;
            }
            while(tmp_type_code instanceof ContainerTypeCode) {
                tmp_type_code = ((ContainerTypeCode)tmp_type_code).getTrueContentTypeCode();
                if(tmp_type_code != null && tmp_type_code.isIsWStringType()) {
                    return true;
                }
            }
        }
        return false;
    }

    public boolean isHasFixedStringMember() {
        for (Member element : getMembers()) {
            if (element.getTypecode() == null) {
                continue;
            }
            TypeCode tmp_type_code = element.getTypecode();
            if(tmp_type_code != null && tmp_type_code.isStringType() && tmp_type_code.isIsBounded()) {
                return true;
            }
            while(tmp_type_code instanceof ContainerTypeCode) {
                tmp_type_code = ((ContainerTypeCode)tmp_type_code).getTrueContentTypeCode();
                if(tmp_type_code != null && tmp_type_code.isStringType() && tmp_type_code.isIsBounded()) {
                    return true;
                }
            }
        }
        return false;
    }

//-------------maxSerializedSize----------------//
    // Add member and the default one at the end.
    public List<Member> getMembersDefaultAtEnd()
    {
        int position = 0;
        List<Member> ret_members = new ArrayList<Member>();
        Member default_member = null;

        for (Member m : getMembers())
        {
            if (position == m_defaultindex)
            {
                default_member = m;
            } else
            {
                if (null != m)
                    ret_members.add(m);
            }
        }

        if (null != default_member)
        {
            ret_members.add(default_member);
        }

        return ret_members;
    }

    @Override
    public long maxSerializedSize(
            long current_alignment)
    {
        long initial_alignment = current_alignment;
        long reset_alignment = 0;
        long union_max_size_serialized = 0;
        com.eprosima.idl.parser.typecode.TypeCode.ExtensibilityKind union_ext_kind = get_extensibility();

        if (!detect_recursive_)
        {
            detect_recursive_ = true;
            current_alignment = MemberedTypeCodeInterface.xcdr_extra_header_serialized_size(current_alignment, union_ext_kind);

            current_alignment += ((TypeCode)getDiscriminator()).maxSerializedSize(current_alignment);

            for (Member member : getMembers())
            {
                reset_alignment = MemberedTypeCodeInterface.xcdr_extra_member_serialized_size(
                        current_alignment,
                        union_ext_kind, member.isAnnotationOptional(),
                        member);
                reset_alignment += ((TypeCode)member.getTypecode()).maxSerializedSize(reset_alignment);
                if (union_max_size_serialized < reset_alignment)
                {
                    union_max_size_serialized = reset_alignment;
                }
            }

            current_alignment = MemberedTypeCodeInterface.xcdr_extra_endheader_serialized_size(union_max_size_serialized, union_ext_kind);
            detect_recursive_ = false;
        }

        return current_alignment - initial_alignment;
    }

    @Override
    public long maxPlainTypeSerializedSize(
            long current_alignment,
            long align64) throws RuntimeGenerationException
    {
        throw new RuntimeGenerationException("UnionTypeCode::maxPlainTypeSerializedSize(): Unions are not plain types.");
    }
//-------------maxSerializedSize----------------//

    @Override
    public String getHashstirng() {
        StringBuilder unionString = new StringBuilder();
        TypeCode switch_true_TypeCode = m_discriminatorTypeCode;
        if (switch_true_TypeCode != null) {
            unionString.append("switch_type ");
            if (switch_true_TypeCode instanceof AliasTypeCode) {
                switch_true_TypeCode = ((AliasTypeCode) switch_true_TypeCode).getTrueContentTypeCode();
            }
            if (switch_true_TypeCode.isIsType_c()) {
                unionString.append(switch_true_TypeCode.getHash()).append(" ");
            } else {
                unionString.append(switch_true_TypeCode.getHashstirng()).append(" ");
            }
        }

        StringBuilder memberString = new StringBuilder();
        for (Member member : super.getMembers()) {
            TypeCode tmp_true_TypeCode = member.getTypecode();
            if (tmp_true_TypeCode instanceof AliasTypeCode) {
                tmp_true_TypeCode = ((AliasTypeCode) tmp_true_TypeCode).getTrueContentTypeCode();
            }
            if (tmp_true_TypeCode.isAnnotationMutable()) {
                continue;
            }
            for (String label : ((UnionMember) member).getLabels()) {
                memberString.append(label).append(" ");
            }
            String inner_hash_string = "";
            if((tmp_true_TypeCode.isPrimitive() && !tmp_true_TypeCode.isIsType_c() && !tmp_true_TypeCode.isIsBitmaskType()) || tmp_true_TypeCode.isIsType_d() ||
                tmp_true_TypeCode.isIsType_f() || tmp_true_TypeCode.isIsSequenceType() || tmp_true_TypeCode.isIsMapType()) {
                //基础类型用名称
                inner_hash_string = tmp_true_TypeCode.getHashstirng();
            } else {
                //复杂类型用hash
                inner_hash_string = tmp_true_TypeCode.getHash();
            }
            memberString.append(inner_hash_string).append(" ").append(((UnionMember) member).getName()).append(" ");
        }

        if (memberString.length() > 0 && memberString.charAt(memberString.length() - 1) == ' ') {
            memberString.setLength(memberString.length() - 1);
        }

        return unionString.toString() + memberString.toString();
    }

    public void computeRustStatementsForDiscriminator() {
        if (m_discriminatorTypeCode != null) {
            Member dummy = new Member(m_discriminatorTypeCode, "discriminator_temp");
            processOneMemberForRust(dummy);
        }
    }

    @Override
    public void computeRustStatements() {
        super.computeRustStatements();
        computeRustStatementsForDiscriminator();
    }

    @Override
    public boolean isUseStatements()
    {
        computeRustStatements();
        return !m_useStatements.isEmpty();
    }

    @Override
    public List<String> getUseStatements()
    {
        computeRustStatements();
        return m_useStatements;
    }

    private TypeCode m_discriminatorTypeCode = null;

    private int m_defaultindex = -1;

    private String m_defaultValue = null;

    private String m_javaDefaultValue = null;
    private static int aliasCounter = 0;
    private int mt_mpValue = 0;
    protected boolean detect_recursive_ = false;
}
