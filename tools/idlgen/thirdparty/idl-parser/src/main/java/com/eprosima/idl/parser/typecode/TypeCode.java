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
// 2023-05 ~ 2025-05: Continuous enhancement of idlgen and idl2xml:
// Support for idl2xml parser, pubsub mode java and jni code generation, RTI idl import and code generation, RPC java and jni generation, java typedef and unsigned type support, python interface update, XML string interface, cpp macro compatibility, tostring/get_type_name interface, union and typeobject improvement, xcdr and large heap support, type validation, data_to_json, @vbsconfig, vbs idl to rti idl, -PackOne pre-selection, uint64 and seq combination support, optimization for concurrent generation and efficiency, and other detailed enhancements, etc.
// ------------------------------------------------------------------

package com.eprosima.idl.parser.typecode;

import com.eprosima.idl.parser.tree.Annotation;
import com.eprosima.idl.parser.tree.AnnotationMember;
import com.eprosima.idl.parser.tree.Notebook;
import com.eprosima.idl.context.Context;
import com.eprosima.idl.parser.exception.RuntimeGenerationException;
import com.eprosima.idl.util.Util;

import java.util.Map;
import java.util.HashMap;
import java.util.Collection;
import java.util.List;
import java.util.ArrayList;
import java.util.regex.Pattern;

import org.antlr.stringtemplate.StringTemplate;
import org.antlr.stringtemplate.StringTemplateGroup;
import org.apache.commons.jexl3.JexlBuilder;
import org.apache.commons.jexl3.JexlEngine;
import org.apache.commons.jexl3.JexlExpression;
import org.apache.commons.jexl3.JexlContext;
import org.apache.commons.jexl3.MapContext;

public abstract class TypeCode implements Notebook
{
    public enum ExtensibilityKind
    {
        NOT_APPLIED(0),
        FINAL(1),
        APPENDABLE(2),
        MUTABLE(3);

        private int value_ = 0;

        private ExtensibilityKind(int value)
        {
            value_ = value;
        }

        public int get_value()
        {
            return value_;
        }
    };

    public static ExtensibilityKind default_extensibility = ExtensibilityKind.APPENDABLE;
    public static StringTemplateGroup idltypesgr  = null;
    public static StringTemplateGroup cpptypesgr  = null;
    public static StringTemplateGroup ctypesgr    = null;
    public static StringTemplateGroup javatypesgr = null;
    public static StringTemplateGroup xmltypesgr = null;
    public static StringTemplateGroup rusttypesgr = null;
    public static StringTemplateGroup rtiidltypesgr = null;
    public static Context ctx = null;
    //TODO Revisar si es el mejor sitio.
    public static String javapackage = "";

    public TypeCode(int kind)
    {
        m_kind = kind;
        m_annotations = new HashMap<String, Annotation>();
    }

    public TypeCode(TypeCode other)
    {
        m_kind = other.m_kind;
        m_annotations = new HashMap<String, Annotation>();
        for (Map.Entry<String, Annotation> entry : other.m_annotations.entrySet()) {
            this.m_annotations.put(entry.getKey(), new Annotation(entry.getValue()));
        }
        this.m_kind = other.m_kind;
        this.m_parent = other.m_parent;
        this.m_forwarded = other.m_forwarded;
        this.m_firstforwarded = other.m_firstforwarded;
        this.m_defined = other.m_defined;
        this.extensibility_ = other.extensibility_;
        this.m_is_default_extensibility = other.m_is_default_extensibility;
    }

    public int getKind()
    {
        return m_kind;
    }

    public boolean isIsAnyTypeCode()
    {
        return m_kind == Kind.KIND_NULL;
    }

    /*|
     * @brief This function returns the typename with the scope that is obtained using the cpptypesgr string template.
     * @return The IDL typename.
     */
    public abstract String getCppTypename();

    /*|
     * @brief This function returns the c typename with the scope that is obtained using the idltypesgr string template.
     */
    public abstract String getCTypename();

    public abstract String getCwrapperTypename();

    protected StringTemplate getCppTypenameFromStringTemplate()
    {
        StringTemplate st = cpptypesgr.getInstanceOf("type_" + Integer.toHexString(m_kind));
        if (m_kind == Kind.KIND_ULONGLONG) {
            st.setAttribute("ctx", ctx);
        }

        return st;
    }

    protected StringTemplate getCTypenameFromStringTemplate()
    {
        StringTemplate st = ctypesgr.getInstanceOf("type_" + Integer.toHexString(m_kind));
        return st;
    }

    private String checkType(String key_normal, boolean is_wrapper)
    {
        if (is_wrapper) {
            switch (key_normal) {
                case "int":
                {
                    return "Integer";
                }
                case "char":
                {
                    return "Character";
                }
                default:
                {
                    return key_normal.substring(0, 1).toUpperCase() + key_normal.substring(1);
                }
            }
        } else {
            if (!isPrimitive()) {
                return "Object";
            } else {
                return key_normal.substring(0, 1).toUpperCase() + key_normal.substring(1);
            }
        }

    }

    private String checkJNIType(String key_normal)
    {
        switch (key_normal) {
            case "void":
            {
                return "void";
            }
            default:
            {
                if (isIsType_a()) {
                    return "jobject";
                } else {
                    return "j"+key_normal.substring(0, 1).toLowerCase() + key_normal.substring(1);
                }
            }
        }
    }

    private String checkJNISign(String key_normal)
    {
        switch (key_normal) {
            case "boolean":
            {
                return "Z";
            }
            case "long":
            {
                return "J";
            }
            case "String":
            {
                return "Ljava/lang/String;";
            }
            default:
            {
                if (isIsType_a()) {
                    return "L"+javapackage.replace('.', '/');
                } else {
                    return key_normal.substring(0, 1).toUpperCase();
                }

            }
        }
    }

    private String checkRetJNISign(String key_normal)
    {
        switch (key_normal) {
            case "boolean":
            {
                return "Z";
            }
            case "long":
            {
                return "J";
            }
            case "String":
            {
                return "LiRPC/utils/Mutable;";
            }
            default:
            {
                if (isIsType_a()) {
                    return "LiRPC/utils/Mutable;";
                } else {
                    return key_normal.substring(0, 1).toUpperCase();
                }

            }
        }
    }

    private String checkRetJNISignNormal(String key_normal)
    {
        switch (key_normal) {
            case "boolean":
            {
                return "Z";
            }
            case "long":
            {
                return "J";
            }
            case "String":
            {
                return "Ljava/lang/String;";
            }
            default:
            {
                if (isIsType_a()) {
                    return key_normal;
                } else {
                    return key_normal.substring(0, 1).toUpperCase();
                }

            }
        }
    }

    private String checkReturntype(String key_normal)
    {
        switch (key_normal) {
            case "int":
            {
                return "0";
            }
            case "double":
            {
                return "0.0";
            }
            case "boolean":
            {
                return "false";
            }
            case "bool":
            {
                return "false";
            }
            case "char":
            {
                return "'A'";
            }
            case "byte":
            {
                return "123";
            }
            case "short":
            {
                return "1000";
            }
            case "long":
            {
                return "1234567890L";
            }
        }
        return "null";
    }

    public static String checkCPPReturnType(String cppType) {
        switch (cppType) {
            case "int":
                return "0";
            case "double":
                return "0.0";
            case "bool":
                return "false";
            case "char":
                return "'\\0'";
            case "byte":
                return "0";
            case "short":
                return "0";
            case "long":
                return "0L";
            case "float":
                return "0.0f";
            case "unsigned int":
                return "0U";
            case "unsigned long":
                return "0UL";
            case "long long":
                return "0LL";
            case "unsigned long long":
                return "0ULL";
            default:
                return "nullptr";
        }
    }

    public abstract String getJavaTypename();

    public abstract String getRustTypename();

    public String getJavaWrapperTypename()
    {
        return checkType(getJavaTypename(), true);
    }

    public String getJavaUpperFirstTypename()
    {
        return checkType(getJavaTypename(), false);
    }

    public String getJNITypename()
    {
        return checkJNIType(getJavaTypename());
    }

    public String getJNISign()
    {
        return checkJNISign(getJavaTypename());
    }

    public String getRetJNISign()
    {
        return checkRetJNISign(getJavaTypename());
    }

    public String getRetJNISignNormal()
    {
        return checkRetJNISignNormal(getJavaTypename());
    }

    public String getJavaDefaultReturn()
    {
        return checkReturntype(getJavaTypename());
    }

    public String getCppDefaultReturn()
    {
        return checkCPPReturnType(getCppTypename());
    }

    protected StringTemplate getJavaTypenameFromStringTemplate()
    {
        StringTemplate st = javatypesgr.getInstanceOf("type_" + Integer.toHexString(m_kind));

        st.setAttribute("package", javapackage);
        return st;
    }

    protected StringTemplate getRustTypenameFromStringTemplate()
    {
        StringTemplate st = rusttypesgr.getInstanceOf("type_" + Integer.toHexString(m_kind));

        st.setAttribute("package", javapackage);
        return st;
    }

    /*|
     * @brief This function returns a typename with scope that is obtained using the m_stringtemplatetypesgr string template.
     * @return The typename.
     */
    public abstract String getIdlTypename();

    public String getIdlFileTypename()
    {
        return getIdlTypename();
    }

    protected StringTemplate getIdlTypenameFromStringTemplate()
    {
        return idltypesgr.getInstanceOf("type_" + Integer.toHexString(m_kind));
    }

    public abstract String getXmlTypename();

    protected StringTemplate getXmlTypenameFromStringTemplate()
    {
        return xmltypesgr.getInstanceOf("type_" + Integer.toHexString(m_kind));
    }

    /*!
     * @brief This function returns the type as a string: "type_2", where the number is the type kind.
     * This function is used in stringtemplates.
     */
    public String getStType()
    {
        return "type_" + Integer.toHexString(m_kind);
    }

    // By default a typecode is not primitive. Function used in stringtemplates
    // TODO Cambiar a isIsPrimitive
    public boolean isPrimitive()
    {
        return false;
    }

    // By default there is not initial value. Function used in stringtemplates.
    public String getInitialValue()
    {
        return "";
    }

    public String getRustInitialValue()
    {
        return "";
    }

    public String getOneLevelContentTypeCodeName()
    {
        return null;
    }

    public String getJavaInitialValue()
    {
        return getInitialValue();
    }

    protected String getInitialValueFromStringTemplate()
    {
        Map initialValues = cpptypesgr.getMap("initialValues");
        return initialValues.get(getStType()).toString();
    }

    // By default a typecode doesn't have a max size limit. Function used in stringtemplates
    public String getMaxsize()
    {
        return null;
    }

    public boolean isMaxsizeGreaterThan1()
    {
        return false;
    }

    public boolean isMaxsizeGreaterThan0()
    {
        return false;
    }

    // By default a typecode doesn't have a max size limit. Function used in stringtemplates
    public String getXMLMaxsize()
    {
        return null;
    }

    /*!
     * @brief This function returns the size of the datatype. By default is null string.
     * @return The size of the datatype.
     */
    protected String getSize()
    {
        return null;
    }

    /*!
     * @brief This function returns the size of the datatype. By default is null string.
     * @return The size of the datatype.
     * note for estimate struct size
     */
    public String getEstimateSize()
    {
        return null;
    }

    public void setUseBBMode(boolean useBBMode) {
    }

    public int getMaxsizeInt()
    {
        return 0;
    }

    public boolean isSetLength()
    {
        return false;
    }

    public boolean isTop() {
        return false;
    }

    public String getScopednamenormal() {
        return getCppTypename();
    }

    public String getScopedname() {
        return getCppTypename();
    }

    public String getCScopednamenormal()
    {
        return getCppTypename();
    }

    public String getCScopedname()
    {
        return getCppTypename();
    }

    public String getScopenormal()
    {
        return "";
    }

    public String getJavaScoped()
    {
        return null;
    }

    public String getBBCppTypename() {
        return getCppTypename();
    }

    public String getHashstirng() {
        return "";
    }

    public String getHash() {
        return Util.stringHashToHex(getHashstirng());
    }
    //public abstract Pair<Integer, Integer> getMaxSerializedSize(int currentSize, int lastDataAligned);

    //public abstract int getMaxSerializedSizeWithoutAlignment(int currentSize);

    /*** Functions to know the type in string templates ***/
    // By default a typecode is not string. Function used in stringtemplates
    public boolean isIsType_1(){return false;}
    public boolean isIsType_2(){return false;}
    public boolean isIsType_3(){return false;}
    public boolean isIsType_4(){return false;}
    public boolean isIsType_5(){return false;}
    public boolean isIsType_6(){return false;}
    public boolean isIsType_7(){return false;}
    public boolean isIsType_8(){return false;}
    public boolean isIsType_9(){return false;}
    public boolean isIsType_b(){return false;}
    public boolean isIsType_d(){return false;}
    public boolean isIsType_c(){return false;}
    public boolean isIsType_f(){return false;}
    public boolean isIsType_e(){return false;}
    public boolean isIsType_a(){return false;}
    public boolean isIsType_10(){return false;}
    public boolean isIsType_11(){return false;}
    public boolean isIsType_12(){return false;}
    public boolean isIsType_13(){return false;}
    public boolean isIsType_15(){return false;}
    public boolean isIsType_19(){return false;}
    public boolean isIsType_1d(){return false;}
    public boolean isIsBitmaskType(){return false;}
    public boolean isIsBitsetType(){return false;}
    public boolean isIsStringType() { return false;}
    public boolean isIsWStringType() { return false;}
    public boolean isIsWCharType() { return false;}
    public boolean isIsSetType() { return false; }
    public boolean isIsMapType() { return false; }
    public boolean isIsSequenceType() { return false; }
    public boolean isIsArrayType() { return false; }
    public boolean isIsStructType() {return isIsType_a(); }
    public boolean isIsUnionType() {return m_kind == Kind.KIND_UNION; }
    public boolean isIsType_uint8() {return false;}
    public boolean isIsType_1e() {return false;}

    public boolean isHasArray() {return false;}
    public boolean isHasArrayInt() {return false;}
    public boolean isHasArrayLong() {return false;}
    public boolean isHasArrayBoolean() {return false;}
    public boolean isHasArrayFloat() {return false;}
    public boolean isHasArrayDouble() {return false;}
    public boolean isHasArrayChar() {return false;}
    public boolean isHasArrayByte() {return false;}
    public boolean isHasArrayShort() {return false;}
    public boolean isHasArrayString() {return false;}

    public boolean isCompatibleOrAppendableOrMutable()
    {
        return isAnnotationCompatible() || isAnnotationAppendable() || isAnnotationMutable();
    }

    public boolean isStructOrUnion()
    {
        return isIsUnionType() || isIsType_a();
    }

    public boolean isSUBS()
    {
        return isIsUnionType() || isIsType_a() || isIsBitsetType();
    }

    public boolean isTrueEnum()
    {
        return (isIsType_10() && getContentTypeCode().isIsType_c()) || isIsType_c();
    }

    public boolean isLikeEnum()
    {
        return isIsBitmaskType() || isIsType_c();
    }

    public boolean isAliasOrStruct()
    {
        return isIsType_10() || isIsType_a();
    }

    public boolean isFloatOrDouble()
    {
        return isIsType_5() || isIsType_6();
    }

    public boolean isContainerTypeCode()
    {
        return isIsSequenceType() || isIsMapType() || isIsSetType() || isIsArrayType();
    }

    public boolean isNeedPrintHelp()
    {
        return isIsType_f() || isIsType_e() || isIsMapType();
    }

    public boolean isHasToDataStringFunc()
    {
        return isSUBS() || isIsBitmaskType() || isIsType_c() || isIsType_7() ||
                isIsType_d() || isIsType_8() || isIsWCharType() || isIsType_1d() ||
                isIsType_5() || isIsType_6() || isIsType_9();
    }

    public boolean isBoundedStringType()
    {
        return (!isIsType_10()) && isStringType() && isIsBounded();
    }

    public boolean isStringType()
    {
        return isIsStringType() || isIsWStringType();
    }

    public boolean isDefault() {
        return isPrimitive() || isStringType();
    }

    public boolean isLikeStruct() {
        return isIsBitmaskType() || isIsBitsetType() || isIsUnionType() || isIsType_c();
    }

    public boolean isCanBePtr() {
        return !(isIsSequenceType() || isIsMapType() || (isIsType_d() && !isIsBounded()));
    }

    // DHEADER if XCDRv2
    public boolean isNeedExcer() {
        return !isPrimitive() && !isIsType_c();
    }

    // Functions to ease TypeIdentifier and TypeObject generation.
    public String getCppTypenameForTypeId() {
        String s = getCppTypename();
        if (s.equals("long double")) {
            return "longdouble";
        } else if (s.equals("unsigned long long")) {
            return "uint64_t";
        }
        return s;
    }

    public String getCppTypenameForTypeIdNormal() {
        String s = getScopednamenormal();
        if (s.equals("long double")) {
            return "longdouble";
        } else if (s.equals("unsigned long long")) {
            return "uint64_t";
        }
        return s;
    }

    public String getTypeIdentifier() { return "TK_None"; }
    public boolean isPrimitiveType() { return false; }
    public boolean isPlainType() { return false; }
    public boolean isObjectType() { return false; }

    // Functions to ease full preallocation and zero-copy
    public boolean isIsPlain() { return true; } // Don't confuse with isPlainType!
    public boolean isIsBounded() { return true; }
    public boolean isUnbound() { return false; }

    public boolean isNotPrimitiveType() {
        return !isPrimitiveType();
    }

    public boolean isUseBBMode() {
        return false;
    }
    /*** End of functions to know the type in string templates ***/

    public Object getParent()
    {
        return m_parent;
    }

    public List<Member> getBBModeMembers()
    {
        return null;
    }

    public void setParent(Object parent)
    {
        m_parent = parent;
    }

    public String getJniScopedname()
    {
        return null;
    }

    public String getJniScope()
    {
        return null;
    }

    public List<Member> getMembers()
    {
        return null;
    }

    public List<TypeCode> getAllInnerMembers()
    {
        return null;
    }

    public String getPname()
    {
        return null;
    }

    public boolean setPname(String pname)
    {
        return false;
    }

    public boolean isSetPname()
    {
        return false;
    }

    public boolean isSetPname(String pname)
    {
        return false;
    }

    public boolean isSetEmptyPname()
    {
        return false;
    }

    public String getEmptyArrayExtension()
    {
        return "";
    }

    public String getLeftEmptyArrayExtension()
    {
        return "";
    }

    public String getRustLeftEmptyArrayExtension()
    {
        return null;
    }

    public String getLeftEmptyArrayExtensionByNum(int size)
    {
        return "";
    }

    void calculate_extensibility(
            ExtensibilityKind base_ext)
    {
        if (ExtensibilityKind.NOT_APPLIED == extensibility_)
        {
            if (m_annotations.containsKey(Annotation.final_str) ||
                    (m_annotations.containsKey(Annotation.extensibility_str) &&
                     m_annotations.get(Annotation.extensibility_str).getValue().equals(Annotation.ex_final_val)))
            {
                extensibility_ = ExtensibilityKind.FINAL;
            }
            else if (m_annotations.containsKey(Annotation.appendable_str) ||
                    (m_annotations.containsKey(Annotation.extensibility_str) &&
                     m_annotations.get(Annotation.extensibility_str).getValue().equals(Annotation.ex_appendable_val)))
            {
                extensibility_ = ExtensibilityKind.APPENDABLE;
            }
            else if (m_annotations.containsKey(Annotation.mutable_str) ||
                    (m_annotations.containsKey(Annotation.extensibility_str) &&
                     m_annotations.get(Annotation.extensibility_str).getValue().equals(Annotation.ex_mutable_val)))
            {
                extensibility_ = ExtensibilityKind.MUTABLE;
            }
            else
            {
                if (ExtensibilityKind.NOT_APPLIED != base_ext)
                {
                    extensibility_ = base_ext;
                }
                else
                {
                    m_is_default_extensibility = true;
                    extensibility_ = default_extensibility;
                }
            }
        }
    }

    public static int evaluateMathExpression(String expression_s) {
        if (expression_s.equals("")) {
            return 0;
        }
        if (isNumeric(expression_s)) {
            long numericValue = Long.parseLong(expression_s);
            if (numericValue == 4294967295L) {
                return -1;
            }
            return Integer.parseInt(expression_s);
        }
        if (isHexNumeric(expression_s)) {
            return Integer.parseInt(expression_s.substring(2), 16);
        }
        try {
            jexlEngine = getJexlEngine();
            JexlExpression expression = jexlEngine.createExpression(expression_s);
            JexlContext context = new MapContext();
            Object result = expression.evaluate(context);

            return result instanceof Number ? ((Number) result).intValue() : 0;
        } catch (Exception e) {
            System.err.println("Error evaluating expression: " + expression_s);
            e.printStackTrace();
            return 0;
        }
    }

    public static boolean isNumeric(String value) {
        return DECIMAL_PATTERN.matcher(value).matches();
    }

    public static boolean isHexNumeric(String value) {
        return HEX_PATTERN.matcher(value).matches();
    }

    @Override
    public void addAnnotation(Context ctx, Annotation annotation)
    {
        if (annotation == null) {
            return;
        }

        if(annotation != null) {
            String annotationName = annotation.getName();
            if ("vbsconfig".equals(annotationName)) {
                Annotation existing = m_annotations.get(annotationName);
                if (existing != null) {
                    existing.setConfigMembers(annotation.getValues());
                } else {
                    m_annotations.put(annotationName, annotation);
                    annotation.setConfigMembers(annotation.getValues());
                }
            } else {
                m_annotations.put(annotationName, annotation);
            }
        }
    }

    @Override
    public Map<String, Annotation> getAnnotations()
    {
        return m_annotations;
    }

    public Collection<Annotation> getAnnotationList()
    {
        return m_annotations.values();
    }

    public boolean isAnnotationFinal()
    {
        if (!ctx.isHasMutable())
            return false;
        calculate_extensibility(ExtensibilityKind.NOT_APPLIED);
        return ExtensibilityKind.FINAL == extensibility_;
    }

    public boolean isAnnotationAppendable()
    {
        if (!ctx.isHasMutable())
            return false;
        calculate_extensibility(ExtensibilityKind.NOT_APPLIED);
        return ExtensibilityKind.APPENDABLE == extensibility_;
    }

    public boolean isAnnotationFinalOrAppendable()
    {
        return isAnnotationFinal() || isAnnotationAppendable();
    }

    public boolean isHasStructAnnotation()
    {
        return isAnnotationFinal() || isAnnotationAppendable() || isAnnotationMutable();
    }

    public boolean isAnnotationMutable()
    {
        if (!ctx.isHasMutable())
            return false;
        calculate_extensibility(ExtensibilityKind.NOT_APPLIED);
        return ExtensibilityKind.MUTABLE == extensibility_;
    }

    public boolean isDefaultExtensibility()
    {
        if (!ctx.isHasMutable())
            return true;
        return m_is_default_extensibility;
    }

    public String getAnnotationVbsConfigSendType() {
        Annotation ann = m_annotations.get("vbsconfig");
        if (ann != null) {
            Map<String, String> suffixMap = ann.getSuffixValue("vbsconfig_value");
            String sendTypeValue = suffixMap.get("SendType");
            if (sendTypeValue != null) {
                String suffixValue = sendTypeValue.toUpperCase();
                switch (suffixValue) {
                    case "ONCHANGE":
                        return "0";
                    default:
                        return suffixValue;
                }
            } else {
                System.err.println("vbsconfig annotation found but missing 'SendType' suffix value");
            }
        }
        return null;
    }

    public String getAnnotationVbsConfigSOALogFilter()
    {
        Annotation ann = m_annotations.get("vbsconfig");
        if (ann != null)
        {
            Map<String, String> suffixMap = ann.getSuffixValue("vbsconfig_value");
            String sendTypeValue = suffixMap.get("SOALogFilter");

            if (sendTypeValue != null) {
                String sufixValue = sendTypeValue.toUpperCase();
                switch (sufixValue) {
                    case "OFF":
                        return "0";
                    case "ON_ENTRANCE":
                        return "1";
                    case "ON_EXPORT":
                        return "2";
                    default:
                        throw new IllegalArgumentException("Unknown SOALogFilter value: " + sufixValue);
                }
            }
        }
        return null;
    }

    public String getAnnotationVbsConfigTopicID()
    {
        Annotation ann = m_annotations.get("vbsconfig");
        if (ann != null) {
            Map<String, String> suffixMap = ann.getSuffixValue("vbsconfig_value");
            String sendTypeValue = suffixMap.get("TopicID");

            if (sendTypeValue != null) {
                return sendTypeValue;
            }
        }
        return null;
    }

    public boolean isAnnotationOptional()
    {
        if (!ctx.isHasMutable())
            return false;
        Annotation ann = m_annotations.get("optional");
        if (ann != null)
        {
            return ann.getValue().toUpperCase().equals("TRUE");
        }
        return false;
    }

    public boolean isAnnotationMutableOrOptional()
    {
        return isAnnotationMutable() || isAnnotationOptional();
    }

    public boolean isAnnotationNested()
    {
        Annotation ann = m_annotations.get("nested");
        if (ann != null)
        {
            return ann.getValue().toUpperCase().equals("TRUE");
        }
        return false;
    }

    public boolean isAnnotationCompatible()
    {
        Annotation ann = m_annotations.get("compatible");
        if (ann != null)
        {
            return ann.getValue().toUpperCase().equals("TRUE");
        }
        return false;
    }

    public boolean isNeedsPointer()
    {
        return m_forwarded || (ctx.isHeapAllocation() && isCanBePtr() && maxSerializedSize(0) > ctx.getSizeUpperLimit());
    }

    public boolean isForwarded()
    {
        return m_forwarded;
    }

    public void setForwarded(boolean fwd)
    {
        m_forwarded = fwd;
    }

    public boolean isFirstForwarded()
    {
        return m_forwarded && m_firstforwarded;
    }

    public void setFirstForwarded(boolean ffwd)
    {
        m_firstforwarded = ffwd;
    }

    public boolean isDefined()
    {
        return m_defined;
    }

    public void setDefined()
    {
        m_defined = true;
    }

    public TypeCode getContentTypeCode()
    {
        return null;
    }

    public List<String> getForListSendBegin(String my_name)
    {
        return null;
    }

    public String getForListSendMid(String my_name)
    {
        return "";
    }

    public List<String> getForListSendEnd(String my_name)
    {
        return null;
    }

    public List<String> getForListGetBegin(String my_name)
    {
        return null;
    }

    public TypeCode getKeyTypeCode()
    {
        return null;
    }

    public TypeCode getValueTypeCode()
    {
        return null;
    }

    public boolean isObjectArray()
    {
        return false;
    }

    public ArrayList<String> getScopednameList()
    {
        return null;
    }

    public String getScope()
    {
        return "";
    }

    public boolean isPackOneAndPlain() {
        return ctx.isUsePackOne() && isIsPlain();
    }

    /*!
     * @brief
     */
    public String getFormatedCppTypename()
    {
        return getCppTypename().replaceAll("::", "_");
    }

    public boolean isInClass() {
        return false;
    }

    public String getForListGetMid(String my_name)
    {
        return "";
    }

    public List<String> getForListGetEnd(String my_name)
    {
        return null;
    }

    public List<Member> getAllLikeStructMembers() {
        return null;
    }

    public ExtensibilityKind get_extensibility()
    {
        return get_extensibility(ExtensibilityKind.NOT_APPLIED);
    }

    public ExtensibilityKind get_extensibility(
            ExtensibilityKind base_ext)
    {
        calculate_extensibility(base_ext);
        return extensibility_;
    }

    public String getNameOrigin() {
        return "";
    }

    protected static JexlEngine getJexlEngine() {
        if (jexlEngine == null) {
            synchronized (TypeCode.class) {
                if (jexlEngine == null) {
                    jexlEngine = new JexlBuilder().create();
                }
            }
        }
        return jexlEngine;
    }
//-------------maxSerializedSize----------------//
    static long cdr_alignment(
            long current_alignment,
            long data_size)
    {
        return (data_size - (current_alignment % data_size)) & (data_size - 1);
    }

    /*
     * Returns the maximum serialized size between XCDRv1 and XCDRv2.
     */
    public long maxSerializedSize(
            long current_alignment) {
        return 0;
    }

    /*
     * Returns the maximum serialized size for a plain Type
     */
    public long maxPlainTypeSerializedSize(
            long current_alignment,
            long align64) throws RuntimeGenerationException {
        return 0;
    }

    public String getMaxSerializedSize()
    {
        return "";
    }

    public String getMaxKeySerializedSize()
    {
        return "";
    }

//-------------maxSerializedSize----------------//
    private int m_kind = Kind.KIND_NULL;

    // Added parent object to typecode because was needed in DDS with our types (TopicsPlugin_gettypecode)
    private Object m_parent = null;

    private HashMap<String, Annotation> m_annotations = null;

    private boolean m_forwarded = false;

    private boolean m_firstforwarded = true;

    private boolean m_defined = false;

    private ExtensibilityKind extensibility_ = ExtensibilityKind.NOT_APPLIED;

    private boolean m_is_default_extensibility = false;

    private static final Pattern DECIMAL_PATTERN = Pattern.compile("-?\\d+");
    private static final Pattern HEX_PATTERN = Pattern.compile("0[xX][0-9a-fA-F]+");
    private static JexlEngine jexlEngine;
}
