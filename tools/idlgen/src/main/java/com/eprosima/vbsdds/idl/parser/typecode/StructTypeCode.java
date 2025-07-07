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
// 2023-09 ~ 2025-05: Supported large heap/sequence, member hiding, static capacity, default validation; fixed memory, performance, compatibility bugs.
// ------------------------------------------------------------------

package com.eprosima.vbsdds.idl.parser.typecode;

import org.antlr.stringtemplate.StringTemplate;

import com.eprosima.idl.parser.typecode.Member;
import com.eprosima.idl.parser.typecode.MemberedTypeCodeInterface;
import com.eprosima.idl.parser.typecode.TypeCode;
import com.eprosima.idl.parser.tree.Annotation;

import com.eprosima.idl.parser.exception.RuntimeGenerationException;
import java.util.ArrayList;

public class StructTypeCode extends com.eprosima.idl.parser.typecode.StructTypeCode
{
    public StructTypeCode(String scope, String name, String namespace)
    {
        super(scope, name, namespace);
        namespace_ = namespace;
    }

    public boolean isHasKey()
    {
        boolean returnedValue = false;

        for (int count = 0; count < getMembers().size() && !returnedValue; ++count)
        {
            Member member = getMembers().get(count);
            Annotation key = member.getAnnotations().get("Key");

            if (key != null)
            {
                String value = key.getValue("value");

                if(value != null && value.equals("true"))
                    returnedValue = true;
            }
            else // Try with lower case
            {
                key = member.getAnnotations().get("key");
                if (key != null)
                {
                    String value = key.getValue("value");

                    if(value != null && value.equals("true"))
                    {
                        returnedValue = true;
                    }
                }
            }
        }

        return returnedValue;
    }

    public void setIsTopic(boolean value)
    {
        istopic_ = value;
    }

    public boolean isIsTopic()
    {
        return istopic_;
    }

    public ArrayList<String> getNamespaces()
    {
        ArrayList<String> namespaces = new ArrayList<String>();
        String scopes = getScope();
        int ch_pos = scopes.indexOf("::");

        while(0 < ch_pos)
        {
            namespaces.add(scopes.substring(0, ch_pos));
            scopes = scopes.substring(ch_pos + 2);
            ch_pos = scopes.indexOf("::");
        }

        if(!scopes.isEmpty())
        {
            namespaces.add(scopes);
        }

        return namespaces;
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

     //-------------maxSerializedSize----------------//
     public String getMaxSerializedSize()
     {
         return Long.toString(maxSerializedSize(0, false, get_extensibility()));
     }

     @Override
     public long maxSerializedSize(
             long current_alignment)
     {
         return maxSerializedSize(current_alignment, false, get_extensibility());
     }

     private long maxSerializedSize(
             long current_alignment,
             boolean only_keys,
             com.eprosima.idl.parser.typecode.TypeCode.ExtensibilityKind struct_ext_kind)
     {
        // System.out.println(ctx.getLastStructureName());
        if (ctx.isTestCrc()) {
            System.out.println(getCppTypename() + "~~~~~~~~~~~~~~~~~~~~~~" + getHash());
        }
        long initial_alignment = current_alignment;

        if (!detect_recursive_)
        {
            detect_recursive_ = true;
            current_alignment = MemberedTypeCodeInterface.xcdr_extra_header_serialized_size(current_alignment, struct_ext_kind);

            // TODO if only_key, get members sorted.
            for (Member member : getAllMembers())
            {
                if (member.isAnnotationNonSerialized())
                {
                    continue;
                }
                if (!(member.getTypecode().isUseBBMode())) {
                    if (only_keys && isHasKey())
                    {
                        if (member.isAnnotationKey())
                        {
                            if (member.getTypecode() instanceof StructTypeCode &&
                                    ((StructTypeCode)member.getTypecode()).isHasKey())
                            {
                                current_alignment +=
                                    ((StructTypeCode)member.getTypecode()).maxSerializedSize(current_alignment, true,
                                    com.eprosima.idl.parser.typecode.TypeCode.ExtensibilityKind.FINAL); // FINAL to avoid calculation
                                                                                                        // of any XCDR header.
                            }
                            else
                            {
                                current_alignment += (member.getTypecode()).maxSerializedSize(current_alignment);
                            }
                        }
                    }
                    else if (!only_keys)
                    {
                        current_alignment = MemberedTypeCodeInterface.xcdr_extra_member_serialized_size(
                                current_alignment,
                                struct_ext_kind, member.isAnnotationOptional(),
                                member);
                        current_alignment += (member.getTypecode()).maxSerializedSize(current_alignment);
                    }
                }
            }

            current_alignment = MemberedTypeCodeInterface.xcdr_extra_endheader_serialized_size(current_alignment, struct_ext_kind);
            detect_recursive_ = false;
        }
        if (ctx.isTestCrc()) {
            System.out.println(getCppTypename() + "++++++++++++++++++++++" + (current_alignment - initial_alignment));
        }
        return current_alignment - initial_alignment;
     }

     public String getMaxKeySerializedSize()
     {
         return Long.toString(maxSerializedSize(0, true,
                     com.eprosima.idl.parser.typecode.TypeCode.ExtensibilityKind.FINAL));
     }

     public String getMaxXCDRv1PlainTypeSerializedSize() throws RuntimeGenerationException
     {
        return Long.toString(maxPlainTypeSerializedSize(0, 8));
     }

     public String getMaxXCDRv2PlainTypeSerializedSize() throws RuntimeGenerationException
     {
         return Long.toString(maxPlainTypeSerializedSize(0, 4));
     }

     @Override
     public long maxPlainTypeSerializedSize(
             long current_alignment,
             long align64) throws RuntimeGenerationException
     {
         if (ExtensibilityKind.FINAL != get_extensibility() && !ctx.isZeroCopy())
         {
             throw new RuntimeGenerationException("StructTypeCode::maxPlainTypeSerializedSize(): only FINAL structures can be plain.");
         }

         long initial_alignment = current_alignment;

         for (com.eprosima.idl.parser.typecode.StructTypeCode parent : getParents()) {
             current_alignment += ((StructTypeCode)parent).maxPlainTypeSerializedSize(current_alignment, align64);
         }

         for (Member member : getMembers())
         {
             if (member.isAnnotationNonSerialized())
             {
                 continue;
             }

             if (member.isIsPlain())
             {
                 current_alignment += (member.getTypecode()).maxPlainTypeSerializedSize(current_alignment, align64);
             }
             else
             {
                 throw new RuntimeGenerationException("StructTypeCode::maxPlainTypeSerializedSize(): A member returned being non-plain.");
             }
         }

         return current_alignment - initial_alignment;
     }
 //-------------maxSerializedSize----------------//
    private boolean istopic_ = true;
    private String namespace_ = "";
}
