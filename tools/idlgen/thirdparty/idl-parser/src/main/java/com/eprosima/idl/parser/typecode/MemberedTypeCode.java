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
// 2023-05 ~ 2025-05: Continuous enhancement of idlgen and idl2xml: supporting pubsub mode java/jni code generation, idl2xml parser, compilation fix for struct and module with the same name, RTI idl import and code generation, Java seq class and unlimited length string, tostring/get_type_name interface, enum as length assignment, xcdr, is_plain interface restoration, typeobject improvement, serialization and length optimization, -hideInternals for structure content hiding, type validation, data_to_json interface fix, etc.
// ------------------------------------------------------------------

package com.eprosima.idl.parser.typecode;

import java.util.List;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Map;
import java.util.LinkedHashMap;
import java.util.Collections;
import java.security.MessageDigest;
import java.util.function.Function;

import com.eprosima.idl.parser.exception.ParseException;
import com.eprosima.idl.parser.exception.RuntimeGenerationException;
import com.eprosima.idl.parser.tree.Annotation;

//import HashSet,set
import java.util.Set;
import java.util.HashSet;
public abstract class MemberedTypeCode extends TypeCode
{
    protected MemberedTypeCode(int kind, String scope, String name, String namespace)
    {
        super(kind);
        int index = name.lastIndexOf("::");

        if (index != -1) {
            m_scope_normal = (scope.equals("") ? "" : scope + "::") + name.substring(0, index);
            m_scope = m_scope_normal.equals("") ? "" : "::" + m_scope_normal;
            m_name = name.substring(index+2);
        } else {
            m_scope = scope.equals("") ? "" : "::" + scope;
            m_scope_normal = scope;
            m_name = name;
        }
        m_name_origin = name;
        m_members = new LinkedHashMap<String, Member>();
        namespace_ = namespace.isEmpty() ? "" : "::" + namespace;
    }

    protected MemberedTypeCode(MemberedTypeCode other)
    {
        super(other);

        this.m_scope = other.m_scope;
        this.m_scope_normal = other.m_scope_normal;
        this.m_name = other.m_name;
        this.m_name_origin = other.m_name_origin;
        this.m_members = new LinkedHashMap<String, Member>();
        for (Map.Entry<String, Member> entry : other.m_members.entrySet()) {
            this.m_members.put(entry.getKey(), new Member(entry.getValue()));
        }
        this.namespace_ = other.namespace_;
    }

    public String getName()
    {
        return m_name;
    }

    public String getNameOrigin()
    {
        return m_name_origin;
    }

    public String getScopednamenormal()
    {
        if(m_scope_normal.isEmpty())
            return m_name;

        return m_scope_normal + "::" + m_name;
    }

    public ArrayList<String> getScopednameList()
    {
        if (m_scope_normal.isEmpty())
            return null;
        String[] namespaces = m_scope_normal.split("::");

        ArrayList<String> namespaceList = new ArrayList<>(Arrays.asList(namespaces));
        return namespaceList;
    }

    public String getScopedname()
    {
        if(m_scope.isEmpty())
            return m_name;

        return m_scope + "::" + m_name;
    }

    public String getROS2Scopedname()
    {
        if(m_scope.isEmpty())
            return m_name;

        return m_scope + "::dds_::" + m_name + "_";
    }

    public String getUnderlineName()
    {
        return m_name.replace("_", "_1");
    }

    public String getUnderlineScopenormal()
    {
        return m_scope_normal.replace("_", "_1");
    }

    public String getUnderlineScopeName()
    {
        if (m_scope_normal.equals(""))
            return getUnderlineName();

        return (m_scope_normal.replace("_", "_1")).replace("::", "_") + "_" + getUnderlineName();
    }

    public String getROS2Scopednamenormal()
    {
        if(m_scope_normal.isEmpty())
            return m_name;

        return m_scope_normal + "::dds_::" + m_name + "_";
    }

    public String getCScopedname()
    {
        if(m_scope.isEmpty())
            return m_name;
        return m_scope.replace("::", "_") + "_" + m_name;
    }

    public String getCScopednamenormal()
    {
        if(m_scope_normal.isEmpty())
            return m_name;
        return m_scope_normal.replace("::", "_") + "_" + m_name;
    }

    public String getJavaScopedname()
    {
        if(m_scope_normal.isEmpty())
            return m_name;

        return m_scope_normal.replace("::", ".") + "." + m_name;
    }

    public String getRustScopedname()
    {
        // if(m_scope.isEmpty())
        //     return "crate::"+m_name;

        // return "crate" + m_scope + "::"+ m_name;
        return m_name;
    }

    public String getJavaScoped()
    {
        if(m_scope_normal.isEmpty())
            return "";

        return m_scope_normal.replace("::", ".") + ".";
    }

    public String getJniScopedname()
    {
        if(m_scope_normal.isEmpty())
            return m_name;

        return m_scope_normal.replace("::", "/") + "/" + m_name;
    }

    public String getJniScope()
    {
        if(m_scope_normal.isEmpty())
            return "";

        return m_scope_normal.replace("::", "/") + "/";
    }

    public String getScope()
    {
        return m_scope;
    }

    public String getScopenormal()
    {
        return m_scope_normal;
    }

    public String getScopenormaltail()
    {
        if (m_scope_normal == null || m_scope_normal.isEmpty()) {
            return "";
        }
        return m_scope_normal + "::";
    }

    public boolean getHasScope()
    {
        return !m_scope.isEmpty();
    }

    public List<Member> getMembers()
    {
        return new ArrayList<Member>(m_members.values());
    }

    public List<TypeCode> getAllInnerMembers()
    {
        ArrayList<TypeCode> all_member_list = new ArrayList<TypeCode>();
        for (Member tmp_member : m_members.values()) {
            if (tmp_member.getTypecode() == null)
                continue;
            if (tmp_member.getTypecode().getKind() == Kind.KIND_STRUCT) {
                List<TypeCode> innerMembers = tmp_member.getTypecode().getAllInnerMembers();
                if (innerMembers != null) {
                    all_member_list.addAll(innerMembers);
                }
            } else if (tmp_member.getTypecode().getKind() == Kind.KIND_UNION) {
                all_member_list.add(((UnionTypeCode)tmp_member.getTypecode()).getDiscriminator());
                List<TypeCode> innerMembers = tmp_member.getTypecode().getAllInnerMembers();
                if (innerMembers != null) {
                    all_member_list.addAll(innerMembers);
                }
            } else if (tmp_member.getTypecode().getKind() == Kind.KIND_ENUM) {

            } else if (tmp_member.getTypecode().getKind() == Kind.KIND_BITSET) {
                for (Member member : ((MemberedTypeCode)tmp_member.getTypecode()).getMembers()) {
                    all_member_list.add(((Bitfield)member).getTypecode());
                }

            } else if (tmp_member.getTypecode().getKind() == Kind.KIND_BITMASK) {

            } else if (tmp_member.getTypecode().getKind() == Kind.KIND_ALIAS) {
                List<TypeCode> innerMembers = tmp_member.getTypecode().getAllInnerMembers();
                if (innerMembers != null) {
                    all_member_list.addAll(innerMembers);
                }
            } else if (tmp_member.getTypecode().getKind() == Kind.KIND_MAP) {
                all_member_list.add(((MapTypeCode)tmp_member.getTypecode()).getKeyTypeCode());
                all_member_list.add(((MapTypeCode)tmp_member.getTypecode()).getValueTypeCode());
            } else if (tmp_member.getTypecode().getKind() == Kind.KIND_SEQUENCE) {
                all_member_list.add(((SequenceTypeCode)tmp_member.getTypecode()).getContentTypeCode());
            } else if (tmp_member.getTypecode().getKind() == Kind.KIND_ARRAY) {
                all_member_list.add(((ArrayTypeCode)tmp_member.getTypecode()).getContentTypeCode());
            }

            all_member_list.add(tmp_member.getTypecode());
        }
        return all_member_list;
    }

    public boolean isHasArray()
    {
        for (Member member : m_members.values())
        {
            if (member.getTypecode().isIsType_f())
            {
                return true;
            } else if (member.getTypecode().isIsType_a())
            {
                if (member.getTypecode().isHasArray())
                {
                    return true;
                }
            }
        }
        return false;
    }

    public boolean isHasArrayInt()
    {
        for (Member member : m_members.values())
        {
            if (member.getTypecode().isIsType_f())
            {
                if (member.getTypecode().getContentTypeCode().isIsType_2())
                    return true;
            } else if (member.getTypecode().isIsType_a())
            {
                if (member.getTypecode().isHasArrayInt())
                {
                    return true;
                }
            }
        }
        return false;
    }

    public boolean isHasArrayLong()
    {
        for (Member member : m_members.values())
        {
            if (member.getTypecode().isIsType_f())
            {
                if (member.getTypecode().getContentTypeCode().isIsType_12())
                    return true;
            } else if (member.getTypecode().isIsType_a())
            {
                if (member.getTypecode().isHasArrayLong())
                {
                    return true;
                }
            }
        }
        return false;
    }

    public boolean isHasArrayBoolean()
    {
        for (Member member : m_members.values())
        {
            if (member.getTypecode().isIsType_f())
            {
                if (member.getTypecode().getContentTypeCode().isIsType_7())
                    return true;
            } else if (member.getTypecode().isIsType_a())
            {
                if (member.getTypecode().isHasArrayBoolean())
                {
                    return true;
                }
            }
        }
        return false;
    }

    @Override
    public boolean isSetPname(String pname)
    {
        for (Member member : m_members.values())
        {
            member.setPname(pname);
        }
        return true;
    }

    @Override
    public boolean isSetEmptyPname()
    {
        for (Member member : m_members.values())
        {
            member.setPname("");
        }
        return true;
    }

    public boolean isHasArrayFloat()
    {
        for (Member member : m_members.values())
        {
            if (member.getTypecode().isIsType_f())
            {
                if (member.getTypecode().getContentTypeCode().isIsType_5())
                    return true;
            } else if (member.getTypecode().isIsType_a())
            {
                if (member.getTypecode().isHasArrayFloat())
                {
                    return true;
                }
            }
        }
        return false;
    }

    public boolean isHasArrayDouble()
    {
        for (Member member : m_members.values())
        {
            if (member.getTypecode().isIsType_f())
            {
                if (member.getTypecode().getContentTypeCode().isIsType_6())
                    return true;
            } else if (member.getTypecode().isIsType_a())
            {
                if (member.getTypecode().isHasArrayDouble())
                {
                    return true;
                }
            }
        }
        return false;
    }

    public boolean isHasArrayChar()
    {
        for (Member member : m_members.values())
        {
            if (member.getTypecode().isIsType_f())
            {
                if (member.getTypecode().getContentTypeCode().isIsType_8())
                    return true;
            } else if (member.getTypecode().isIsType_a())
            {
                if (member.getTypecode().isHasArrayChar())
                {
                    return true;
                }
            }
        }
        return false;
    }

    public boolean isHasArrayByte()
    {
        for (Member member : m_members.values())
        {
            if (member.getTypecode().isIsType_f())
            {
                if (member.getTypecode().getContentTypeCode().isIsType_9())
                    return true;
            } else if (member.getTypecode().isIsType_a())
            {
                if (member.getTypecode().isHasArrayByte())
                {
                    return true;
                }
            }
        }
        return false;
    }

    public boolean isHasArrayShort()
    {
        for (Member member : m_members.values())
        {
            if (member.getTypecode().isIsType_f())
            {
                if (member.getTypecode().getContentTypeCode().isIsType_1() || member.getTypecode().getContentTypeCode().isIsType_3())
                    return true;
            } else if (member.getTypecode().isIsType_a())
            {
                if (member.getTypecode().isHasArrayShort())
                {
                    return true;
                }
            }
        }
        return false;
    }

    public boolean isHasArrayString()
    {
        for (Member member : m_members.values())
        {
            if (member.getTypecode().isIsType_f())
            {
                if (member.getTypecode().getContentTypeCode().isIsStringType())
                    return true;
            } else if (member.getTypecode().isIsType_a())
            {
                if (member.getTypecode().isHasArrayString())
                {
                    return true;
                }
            }
        }
        return false;
    }

    /*!
     * This function checks there is no other member with same id.
     */
    protected boolean check_unique_member_id(
            Member member)
    {
        for(Member m : m_members.values())
        {
            if (m.get_id() == member.get_id())
            {
                return false;
            }
        }

        return true;
    }

    public boolean addMember(Member member) throws ParseException
    {
        if (member.isAnnotationOptional() && Kind.KIND_STRUCT != getKind())
        {
            throw new ParseException(null, "Error in member " + member.getName() +
                    ": @optional annotations only supported for structure's members.");
        }
        if (member.isAnnotationExternal() && (
                    Kind.KIND_STRUCT != getKind() && Kind.KIND_UNION != getKind()))
        {
            throw new ParseException(null, "Error in member " + member.getName() +
                    ": @external annotations only supported for structure's members or union's members.");
        }
        if (member.isAnnotationMustUnderstand() && Kind.KIND_STRUCT != getKind())
        {
            throw new ParseException(null, "Error in member " + member.getName() +
                    ": @must_understand annotations only supported for structure's members.");
        }
        if (member.isAnnotationNonSerialized() && Kind.KIND_STRUCT != getKind())
        {
            throw new ParseException(null, "Error in member " + member.getName() +
                    ": @non_serialized annotations only supported for structure's members.");
        }
        if (member.isAnnotationKey() && Kind.KIND_STRUCT != getKind())
        {
            throw new ParseException(null, "Error in member " + member.getName() +
                    ": @" + Annotation.key_str + " annotations only supported for structure's members (Union discriminator still pending implementation).");
        }
        if (null != member.getAnnotationBitBound() && (
                    Kind.KIND_ENUM != getKind() && Kind.KIND_BITMASK != getKind()))
        {
            throw new ParseException(null, "Error in member " + member.getName() +
                    ": @bit_bound annotations only supported for enumeration's members or bitmask's members.");
        }
        if (member.isAnnotationDefaultLiteral() && Kind.KIND_ENUM != getKind())
        {
            throw new ParseException(null, "Error in member " + member.getName() +
                    ": @default_literal annotations only supported for enumeration's members.");
        }
        if (null != member.getAnnotationValue() && Kind.KIND_ENUM != getKind())
        {
            throw new ParseException(null, "Error in member " + member.getName() +
                    ": @value annotations only supported for enumeration's members.");
        }
        if (null != member.getAnnotationPosition() && Kind.KIND_BITMASK != getKind())
        {
            throw new ParseException(null, "Error in member " + member.getName() +
                    ": @position annotations only supported for bitmask's members.");
        }
        if(member.isAnnotationKey() && member.isAnnotationNonSerialized())
        {
            throw new ParseException(null, "Error in member " + member.getName() +
                    ": @" + Annotation.key_str + " and @non_serialized annotations are incompatible.");
        }
        if(member.isAnnotationKey() && member.isAnnotationOptional())
        {
            throw new ParseException(null, "Error in member " + member.getName() +
                    ": @" + Annotation.key_str + " and @optional annotations are incompatible.");
        }
        if (member.isAnnotationId() && (
                    Kind.KIND_STRUCT != getKind() && Kind.KIND_UNION != getKind()))
        {
            throw new ParseException(null, "Error in member " + member.getName() +
                    ": @" + Annotation.id_str +
                    " annotations only supported for structure's members or union's members.");
        }
        if (member.isAnnotationHashid() && (
                    Kind.KIND_STRUCT != getKind() && Kind.KIND_UNION != getKind()))
        {
            throw new ParseException(null, "Error in member " + member.getName() +
                    ": @" + Annotation.hashid_str +
                     "annotations only supported for structure's members or union's members.");
        }
        if (member.isAnnotationId() && member.isAnnotationHashid())
        {
            throw new ParseException(null, "Error in member " + member.getName() +
                    ": @" + Annotation.id_str + " and @" + Annotation.hashid_str + " annotations cannot be together.");
        }

        if(!m_members.containsKey(member.getName()))
        {
            if (Member.MEMBER_ID_INVALID != member.get_id() && !check_unique_member_id(member))
            {
                throw new ParseException(null, member.getName() + " has a MemberId already in use.");
            }
            member.set_index(last_index_++);
            last_id_ = member.get_id();
            m_members.put(member.getName(), member);

            return true;
        }
        return false;
    }

    public boolean isAnnotationAutoid()
    {
        return null != getAnnotations().get(Annotation.autoid_str);
    }

    protected void calculate_member_id_(
            Member member)
    {
        Function<String, Integer> calculate_hash = (String member_name) ->
        {
            int hash_id = 0;
            try {
                byte[] bytes = member_name.getBytes("UTF-8");
                MessageDigest md = MessageDigest.getInstance("MD5");
                byte[] md5 = md.digest(bytes);
                hash_id = ((md5[3] & 0xFF) << 24) + ((md5[2] & 0xFF) << 16) + ((md5[1] & 0xFF) << 8) + (md5[0] & 0xFF);
                hash_id &= 0x0FFFFFFF;
            }
            catch(Exception e)
            {
                e.printStackTrace();
            }
            return hash_id;
        };

        try
        {
            if (member.isAnnotationId())
            {
                member.set_id(Integer.parseInt(member.getAnnotationIdValue()));
            }
            else if (member.isAnnotationHashid())
            {
                String value = member.getAnnotationHashidValue();
                member.set_id(calculate_hash.apply(value.isEmpty() ? member.getName() : value));
            }
            else if (!isAnnotationAutoid() || getAnnotationAutoidValue().equals(Annotation.autoid_sequential_value_str))
            {
                member.set_id(++last_id_);
            }
            else
            {
                member.set_id(calculate_hash.apply(member.getName()));
            }
        }
        catch (RuntimeGenerationException ex)
        {
            // Should be never called because was previously called isAnnotationId() or similar.
        }
    }

    @Override
    public abstract String getCppTypename();

    @Override
    public abstract String getCTypename();

    @Override
    public abstract String getJavaTypename();

    @Override
    public abstract String getRustTypename();

    @Override
    public abstract String getIdlTypename();

    @Override
    public abstract String getXmlTypename();

    @Override
    public boolean isIsPlain()
    {
        if (!ctx.isHasMutable() || isAnnotationFinal() || ctx.isZeroCopy())
        {
            for (Member member : m_members.values())
            {
                if (!member.isIsPlain())
                {
                    return false;
                }
            }

            return true;
        }

        return false;
    }

    @Override
    public boolean isIsBounded()
    {
        for (Member member : m_members.values())
        {
            if (!member.isIsBounded())
            {
                return false;
            }
        }
        return true;
    }

    public boolean isHasAnnotationOptional() {
        for (Member member : m_members.values())
        {
            if (member.isAnnotationOptional())
            {
                return true;
            }
        }
        return false;
    }

    public boolean isEmpty()
    {
        return m_members.size() == 0;
    }

    public String getNamespace() {
        return namespace_;
    }

    public boolean isHasNamespace() {
        return !namespace_.isEmpty();
    }

    public boolean isInClass() {
        return !namespace_.equals(getScope());
    }

    public boolean isNotTempSize() {

        for(Member member : m_members.values()) {
            boolean temp = !member.getTypecode().isPrimitiveType();
            if (temp) {
                return true;
            }
        }
        return false;
    }

    public ArrayList<TypeCode> getSwapedContainerTypedef() {
        ArrayList<TypeCode> result = new ArrayList<TypeCode>();
        result = getContainerTypedef();
        Collections.reverse(result);
        return result.size() == 0 ? null : result;
    }

    public ArrayList<TypeCode> getContainerTypedef() {
        ArrayList<TypeCode> result = new ArrayList<TypeCode>();
        for (Member element : getMembers()) {
            if(element.getTypecode() instanceof ContainerTypeCode) {
                ContainerTypeCode container = (ContainerTypeCode)element.getTypecode();
                if(container.isIsType_10()) {
                    continue;
                }
                if ((container.isInnerContainer())) {
                    if (container.isHasInnerBoundedString()) {
                        element.setXMLTypedeftype();
                        result.addAll(container.getInnerContainer());
                    }
                } else {
                    if (container != null &&
                        container.getContentTypeCode() != null &&
                        (!container.getContentTypeCode().isIsType_10()) &&
                        container.getContentTypeCode().isBoundedStringType()) {
                        element.setXMLTypedeftype();
                        result.add(element.getTypecode());
                        result.add(container.getContentTypeCode());
                    } else if (container != null &&
                               container.isIsMapType() &&
                               (!container.getKeyTypeCode().isIsType_10()) &&
                               container.getKeyTypeCode().isBoundedStringType()) {
                        element.setXMLTypedeftype();
                        result.add(container.getKeyTypeCode());
                    } else if (container != null &&
                               container.isIsMapType() &&
                               (!container.getValueTypeCode().isIsType_10()) &&
                               container.getValueTypeCode().isBoundedStringType()) {
                        element.setXMLTypedeftype();
                        result.add(container.getValueTypeCode());
                    }
                }
            }
        }
        return result;
    }

    public String getAnnotationAutoidValue() throws RuntimeGenerationException
    {
        Annotation ann = getAnnotations().get(Annotation.autoid_str);
        if (ann == null)
        {
            throw new RuntimeGenerationException("Error in member " + m_name + ": @" + Annotation.autoid_str +
                    " annotation not found.");
        }

        return ann.getValue();
    }

    public boolean getContainsOptional()
    {
        for (Member member : m_members.values())
        {
            if (member.isAnnotationOptional())
            {
                return true;
            }
        }
        return false;
    }

    private String computeRelativePath(String fromNamespace, String toNamespace) {
        fromNamespace = fromNamespace.replaceFirst("^::", "");
        toNamespace = toNamespace.replaceFirst("^::", "");

        if (fromNamespace.equals(toNamespace)) {
            return "";
        }

        String[] fromParts = fromNamespace.isEmpty() ? new String[0] : fromNamespace.split("::");
        String[] toParts = toNamespace.isEmpty() ? new String[0] : toNamespace.split("::");

        int commonLength = 0;
        while (commonLength < fromParts.length && commonLength < toParts.length
                && fromParts[commonLength].equals(toParts[commonLength])) {
            commonLength++;
        }

        int upLevels = fromParts.length - commonLength;

        StringBuilder relativePath = new StringBuilder();
        for (int i = 0; i < upLevels; i++) {
            relativePath.append("super::");
        }

        for (int i = commonLength; i < toParts.length; i++) {
            relativePath.append(toParts[i]);
            if (i < toParts.length - 1) {
                relativePath.append("::");
            }
        }

        return relativePath.toString();
    }

    private String generateUseStatement(String paramNamespace, String typeName, com.eprosima.idl.parser.tree.Module paramModule) {
        String fromNamespace = getScope();
        String relativePath = computeRelativePath(fromNamespace, paramNamespace);

        if (relativePath.isEmpty()) {
            return "";
        }
        if (RUST_PRIMITIVES_BUILTINS.contains(typeName)) {
            return "";
        }
        // String importStatement = "use " + relativePath + "::" + typeName + ";";
        // return importStatement;
        String separator = relativePath.endsWith("::") ? "" : "::";
        return "use " + relativePath + separator + typeName + ";";
    }
    protected void processOneMemberForRust(Member param) {
        if (param == null || param.getTypecode() == null) {
            return;
        }
        String paramNamespace = param.getTypecode().getScope();
        if (paramNamespace == null) {
            paramNamespace = "";
        } else {
            paramNamespace = paramNamespace.replaceFirst("^::", "");
        }
        TypeCode type = param.getTypecode();
        com.eprosima.idl.parser.tree.Module module = type.ctx.existsModule(paramNamespace);
        if (module != null) {
            if (!paramNamespace.equals(getScope())) {
                String rustTypename  = param.getTypecode().getRustTypename();
                String importStatement = generateUseStatement(paramNamespace, rustTypename, module);

                if (!importStatement.isEmpty() && !m_useStatements.contains(importStatement)) {
                    m_useStatements.add(importStatement);
                }
            }
        }
    }

    public void computeRustStatements() {
        m_useStatements = new ArrayList<>();
        for (Member param : getMembers()) {
            processOneMemberForRust(param);
        }
    }

    public boolean isUseStatements() {
        computeRustStatements();
        return !m_useStatements.isEmpty();
    }

    public List<String> getUseStatements() {
        computeRustStatements();
        return m_useStatements;
    }

    private String namespace_ = "";

    private String m_name = null;

    private String m_name_origin = null;

    private String m_scope = null;

    private String m_scope_normal = null;

    private LinkedHashMap<String, Member> m_members = null;

    protected int last_index_ = 0;

    protected int last_id_ = -1;

    protected List<String> m_useStatements = null;

    private static final Set<String> RUST_PRIMITIVES_BUILTINS = new HashSet<>(
        Arrays.asList("String", "bool", "u8", "i8", "u16", "i16",
                      "u32", "i32", "u64", "i64", "f32", "f64")
    );
}
