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
// 2023-05 ~ 2025-05: Continuous enhancement of idlgen: supporting idl2xml parser, rpc build optimization, tostring/get_type_name interfaces, xcdr, restore is_plain interface, zero copy support for variable length seq, -hideInternals to hide structure contents, vbs-dependency free data structures, type validation, data_to_json, @autoid improvement, cpp zero copy size estimation, ContainerPrintHelpers removal, abi compatibility optimization, etc.
// ------------------------------------------------------------------

package com.eprosima.idl.parser.typecode;
import com.eprosima.idl.parser.tree.Inherits;
import com.eprosima.idl.context.Context;
import org.antlr.stringtemplate.StringTemplate;

import java.util.List;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.Map;
import java.util.HashMap;
import java.util.LinkedHashSet;
import java.util.Set;

public class StructTypeCode extends MemberedTypeCode implements Inherits, MemberedTypeCodeInterface
{
    public StructTypeCode(String scope, String name, String namespace)
    {
        super(Kind.KIND_STRUCT, scope, name, namespace);
        superTypes_ = new ArrayList<StructTypeCode>();
        useBBMode_ = false;
        isTop_ = false;
    }

    @Override
    public String getTypeIdentifier()
    {
        return "EK_COMPLETE";
    }

    public boolean isHasKey()
    {
        boolean returnedValue = false;

        for (StructTypeCode parent : getParents()) {
            returnedValue |= (parent).isHasKey();
        }

        for (int count = 0; count < getMembers().size() && !returnedValue; ++count)
        {
            Member member = getMembers().get(count);
            returnedValue = member.isAnnotationKey();
        }

        return returnedValue;
    }

    @Override
    public boolean isObjectType() { return true; }

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

        StringTemplate st = getCTypenameFromStringTemplate();
        st.setAttribute("name", getCScopedname());
        return st.toString()+"Wrapper";
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

    @Override
    public boolean isIsType_a(){
        return true;
    }

    @Override
    public void addInheritance(Context ctx, TypeCode parent)
    {
        if (parent instanceof StructTypeCode)
        {
            for (Member member:((StructTypeCode)parent).getAllMembers()) {
                super.last_index_++;
            }
            superTypes_.add((StructTypeCode)parent);
        }
    }

    @Override
    public ArrayList<TypeCode> getInheritances()
    {
        ArrayList<TypeCode> result = new ArrayList<TypeCode>();
        for (StructTypeCode parent : superTypes_)
        {
            result.add(parent);
        }
        return result;
    }

    public ArrayList<String> getInheritancesName()
    {
        ArrayList<String> result = new ArrayList<String>();
        for (StructTypeCode parent : superTypes_)
        {
            result.add(parent.getScopedname());
        }
        if (!ctx.isPureStructure()) {
            if (ctx.isRpc() && superTypes_.size() == 0) {
                result.add("vbs::rpc::RpcMessageType");
            }
        }

        return result;
    }

    public ArrayList<StructTypeCode> getParents()
    {
        return superTypes_;
    }

    public boolean isHasInheritance()
    {
        return superTypes_.size() != 0;
    }

    @Override
    public List<Member> getMembers()
    {
        return getMembers(false);
    }

    @Override
    public List<Member> getBBModeMembers()
    {
        List<Member> allMembers = new ArrayList<Member>();

        for (Member member : super.getMembers()) {
            if (member.getTypecode().isUseBBMode()) {
                allMembers.add(member);
            }
        }
        return allMembers.size() > 0 ? allMembers : null;
    }

    public List<Member> getMembers(boolean includeParents)
    {
        List<Member> allMembers = new ArrayList<Member>();

        if (includeParents)
        {
            for (StructTypeCode p : superTypes_)
            {
                allMembers.addAll(p.getMembers());
            }
        }
        allMembers.addAll(super.getMembers());
        return allMembers;
    }

    public List<Member> getAllMembers() // Alias for getMembers(true) for stg
    {
        return getMembers(true);
    }

    @Override
    public boolean addMember(
            Member member)
    {
        calculate_member_id_(member);
        return super.addMember(member);
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

    public int getAllMembersMaxLength()
    {
        int maxLength = 0;
        for (Member element : getMembers(true)) {
            int tmpLength = evaluateMathExpression(element.getTypecode().getEstimateSize() == null ? "0" : element.getTypecode().getEstimateSize());
            maxLength = maxLength < tmpLength ? tmpLength : maxLength;
        }
        return maxLength;
    }

    @Override
    public boolean isIsPlain()
    {
        boolean returned_value = true;

        if (!detect_recursive_)
        {
            detect_recursive_ = true;
            for (StructTypeCode parent : getParents()) {
                returned_value &= (parent).isIsPlain();
            }
            returned_value &= super.isIsPlain();
            detect_recursive_ = false;
        }
        else {
            returned_value = false;
        }
        return returned_value;
    }

    @Override
    public boolean isIsBounded()
    {
        for (StructTypeCode parent : superTypes_)
        {
            if (!parent.isIsBounded())
            {
                return false;
            }
        }
        return super.isIsBounded();
    }

    @Override
    public String getEstimateSize() {
        int currentOffset = 0;
        int maxAlignment = 1;

        for (Member element : getMembers(false)) {
            String sizeExpr = element.getTypecode().getEstimateSize();
            int memberSize = evaluateMathExpression(sizeExpr == null ? "0" : sizeExpr);

            if (memberSize == 0) {
                continue;
            }

            int memberAlignment = getAlignmentRequirement(element);

            maxAlignment = Math.max(maxAlignment, memberAlignment);

            int misalignment = currentOffset % memberAlignment;
            int padding = (memberAlignment - misalignment) % memberAlignment;

            currentOffset += padding;

            currentOffset += memberSize;
        }

        int misalignment = currentOffset % maxAlignment;
        int finalPadding = (maxAlignment - misalignment) % maxAlignment;

        currentOffset += finalPadding;
        return String.valueOf(currentOffset);
    }

    private int getAlignmentRequirement(Member member) {
        TypeCode elementType = getElementTypeIfArray(member.getTypecode());

        String sizeExpr = elementType.getEstimateSize();
        int elementSize = evaluateMathExpression(sizeExpr == null ? "0" : sizeExpr);

        return Math.min(elementSize, 8);
    }

    private TypeCode getElementTypeIfArray(TypeCode typeCode) {
        if (typeCode instanceof ArrayTypeCode) {
            return ((ArrayTypeCode) typeCode).getContentTypeCode();
        }
        return typeCode;
    }

    public void setUseBBMode(boolean useBBMode) {
        useBBMode_ = useBBMode;
    }

    @Override
    public boolean isUseBBMode() {
        return useBBMode_;
    }

    public void setTopStruct(boolean isTop) {
        isTop_ = isTop;
    }

    @Override
    public boolean isTop() {
        return isTop_;
    }

    @Override
    public boolean isSetLength()
    {
        for (Member element : getMembers(true)) {
            if(element.getTypecode() != null &&!element.getTypecode().isSetLength()) {
                return false;
            }
        }
        return true;
    }

    private static int getNextAlias(int current) {
        aliasCounter = (current + 1);
        return (current + 1);
    }

    public boolean isHasSeqMember() {
        for (Member element : getMembers(false)) {
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
        for (Member element : getMembers(false)) {
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
                if((tmp_type_code != null && (tmp_type_code.isIsArrayType() ||
                    (tmp_type_code.isIsSequenceType() && ctx.isZeroCopy() && !((SequenceTypeCode)tmp_type_code).isUnbound())))) {
                    return true;
                }
            }
        }
        return false;
    }

    public boolean isHasMapMember() {
        for (Member element : getMembers(false)) {
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
        for (Member element : getMembers(false)) {
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
        for (Member element : getMembers(false)) {
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

    @Override
    public String getHashstirng() {
        StringBuilder tmpHashString = new StringBuilder();
        if (superTypes_.size() != 0) {
            tmpHashString.append("parent ");
            for (StructTypeCode parent : superTypes_) {
                tmpHashString.append(parent.getHash()).append(" ");
            }
        }
        for (Member member : super.getMembers()) {
            TypeCode tmp_true_TypeCode = member.getTypecode();
            if(tmp_true_TypeCode instanceof AliasTypeCode) {
                tmp_true_TypeCode = ((AliasTypeCode)tmp_true_TypeCode).getTrueContentTypeCode();
            }

            tmpHashString.append(member.getHashstirng())
                         .append(" ");
        }
        if (tmpHashString.length() > 0) {
            tmpHashString.setLength(tmpHashString.length() - 1);
        }

        return tmpHashString.toString();
    }

    private ArrayList<StructTypeCode> superTypes_;
    private boolean useBBMode_;
    private boolean isTop_;
    private static int aliasCounter = 0;
    protected boolean detect_recursive_ = false;
}
