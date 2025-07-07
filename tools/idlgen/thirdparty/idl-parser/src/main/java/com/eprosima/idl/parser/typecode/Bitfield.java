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
// 2023-05 ~ 2025-05: Continuous optimization of idlgen and idl2xml: support for idl2xml parser, idl2xml bug fix, typeobject improvement, etc.
// ------------------------------------------------------------------

package com.eprosima.idl.parser.typecode;

public class Bitfield extends Member
{
    public Bitfield()
    {
        super();
    }

    public Bitfield(BitsetTypeCode typecode, BitfieldSpec spec, String name)
    {
        super(typecode, name);
        m_spec = spec;
    }

    public Bitfield(Bitfield other)
    {
        super(other);
        m_spec = other.getSpec();
        m_base = other.m_base;
    }

    public Bitfield copy() {
        return new Bitfield(this);
    }

    public void setBasePosition(int position)
    {
        m_base = position;
    }

    public int getBasePosition()
    {
        return m_base;
    }

    public BitfieldSpec getSpec()
    {
        return m_spec;
    }

    @Override
    public String getName()
    {
        if (super.getName().isEmpty())
        {
            return null;
        }
        return super.getName();
    }

    public boolean isIsPlain()
    {
        return false;
    }

    public boolean isIsBounded()
    {
        return false;
    }

    private int m_base = -1;
    private BitfieldSpec m_spec = null;
}
