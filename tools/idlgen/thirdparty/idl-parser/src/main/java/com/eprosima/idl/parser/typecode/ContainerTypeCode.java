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
// 2023-05 ~ 2025-05: Enhancement of idlgen and idl2xml: supporting idl2xml parser, to_idl_string interface fix, adaptation to seq/array with nested bounded strings and length logic, typeobject improvement, Java multi-level typedef array fix, etc.
// ------------------------------------------------------------------

package com.eprosima.idl.parser.typecode;

import com.eprosima.idl.parser.tree.Definition;

import java.util.ArrayList;
public abstract class ContainerTypeCode extends TypeCode
{
    protected ContainerTypeCode(int kind)
    {
        super(kind);
    }

    protected ContainerTypeCode(ContainerTypeCode other)
    {
        super(other);
        m_contentTypeCode = other.m_contentTypeCode;
        m_contentDefinition = other.m_contentDefinition;
    }

    @Override
    public abstract String getCppTypename();

    @Override
    public abstract String getCTypename();

    @Override
    public abstract String getIdlTypename();

    @Override
    public abstract String getXmlTypename();

    public TypeCode getContentTypeCode()
    {
        return m_contentTypeCode;
    }

    public TypeCode getTrueContentTypeCode()
    {
        return m_contentTypeCode;
    }

    public TypeCode getNoAliasContentTypeCode()
    {
        if (m_contentTypeCode instanceof AliasTypeCode) {
            return ((AliasTypeCode)m_contentTypeCode).getNoAliasContentTypeCode();
        }

        return m_contentTypeCode;
    }

    public TypeCode getInnerContentTypeCode()
    {
        if (m_contentTypeCode instanceof AliasTypeCode) {
            return ((AliasTypeCode)m_contentTypeCode).getInnerContentTypeCode();
        }
        if (m_contentTypeCode instanceof ArrayTypeCode) {
            return ((ArrayTypeCode)m_contentTypeCode).getInnerContentTypeCode();
        }
        return m_contentTypeCode;
    }

    public Definition getContentDefinition()
    {
        return m_contentDefinition;
    }

    public void setContentTypeCode(TypeCode contentTypeCode)
    {
        m_contentTypeCode = contentTypeCode;
    }

    public void setContentDefinition(Definition contentDefinition)
    {
        m_contentDefinition = contentDefinition;
    }

    public int getDepth()
    {
        int ret = 1;

        if (m_contentTypeCode.isPrimitive()) {
    	    return ret;
    	} else {
    	    if (m_contentTypeCode instanceof ContainerTypeCode) {
    		    ret += ((ContainerTypeCode) m_contentTypeCode).getDepth();
    		}
    	}

        return ret;
    }

    @Override
    public boolean isIsPlain()
    {
        if (m_contentTypeCode != null)
        {
            return m_contentTypeCode.isIsPlain();
        }
        return false;
    }

    @Override
    public boolean isIsBounded()
    {
        if (m_contentTypeCode != null)
        {
            return m_contentTypeCode.isIsBounded();
        }
        return false;
    }

    public boolean isInnerContainer()
    {
        if(m_contentTypeCode instanceof ContainerTypeCode ||
            (this.isIsMapType() && (((MapTypeCode)this).getKeyTypeCode() instanceof ContainerTypeCode) &&
            (((MapTypeCode)this).getKeyTypeCode() != null && (!((MapTypeCode)this).getKeyTypeCode().isIsType_10()))) ||
            (this.isIsMapType() && (((MapTypeCode)this).getValueTypeCode() instanceof ContainerTypeCode) &&
            (((MapTypeCode)this).getValueTypeCode() != null && (!((MapTypeCode)this).getValueTypeCode().isIsType_10())))) {
            if(m_contentTypeCode != null && m_contentTypeCode.isIsType_10()) {
                return false;
            }
            return true;
        }
        return false;
    }

    public boolean isHasInnerBoundedString()
    {
        //TODO:support map key
        if(this.isIsMapType()) {
            if(((MapTypeCode)this).getKeyTypeCode().isBoundedStringType() ||
                ((MapTypeCode)this).getValueTypeCode().isBoundedStringType()) {
                return true;
            }
            return (((MapTypeCode)this).getValueTypeCode() instanceof ContainerTypeCode &&
                        ((ContainerTypeCode)((MapTypeCode)this).getValueTypeCode()).isHasInnerBoundedString()) ||
                    (((MapTypeCode)this).getKeyTypeCode() instanceof ContainerTypeCode &&
                        ((ContainerTypeCode)((MapTypeCode)this).getKeyTypeCode()).isHasInnerBoundedString());
        } else if(m_contentTypeCode instanceof ContainerTypeCode) {
            if(m_contentTypeCode.isIsType_10()) {
                return false;
            }
            return ((ContainerTypeCode)m_contentTypeCode).isHasInnerBoundedString();
        } else if(m_contentTypeCode.isBoundedStringType()){
            return true;
        }
        return false;
    }

    public ArrayList<TypeCode> getInnerContainer() {
        ArrayList<TypeCode> result = new ArrayList<TypeCode>();
        result.add(this);
        if (this.isIsMapType()) {
            MapTypeCode tmp_map_type = (MapTypeCode)this;
            if (tmp_map_type == null)
                return result;
            if (tmp_map_type.getKeyTypeCode() instanceof ContainerTypeCode &&
                (!tmp_map_type.isIsType_10()) &&
                ((ContainerTypeCode)(tmp_map_type.getKeyTypeCode())).isHasInnerBoundedString()) {
                result.addAll(((ContainerTypeCode)(tmp_map_type.getKeyTypeCode())).getInnerContainer());
            } else if (tmp_map_type.getKeyTypeCode().isBoundedStringType()) {
                result.add(tmp_map_type.getKeyTypeCode());
            }

            if (tmp_map_type.getValueTypeCode() instanceof ContainerTypeCode &&
                (!tmp_map_type.isIsType_10()) &&
                ((ContainerTypeCode)(tmp_map_type.getValueTypeCode())).isHasInnerBoundedString()) {
                result.addAll(((ContainerTypeCode)(tmp_map_type.getValueTypeCode())).getInnerContainer());
            } else if (tmp_map_type.getValueTypeCode().isBoundedStringType()) {
                result.add(tmp_map_type.getValueTypeCode());
            }
        } else if(m_contentTypeCode instanceof ContainerTypeCode) {
            if(m_contentTypeCode.isIsType_10()) {
                return result;
            }
            result.addAll(((ContainerTypeCode)m_contentTypeCode).getInnerContainer());
        } else if(m_contentTypeCode.isBoundedStringType()){
            result.add(m_contentTypeCode);
        }

        return result;
    }

    private TypeCode m_contentTypeCode = null;
    private Definition m_contentDefinition = null;
}
