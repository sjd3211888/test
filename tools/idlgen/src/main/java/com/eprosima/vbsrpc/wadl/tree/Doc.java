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

package com.eprosima.vbsrpc.wadl.tree;

import java.util.ArrayList;

public class Doc
{
	private String m_title = null;
	private String m_ref = null;
	private ArrayList<String> m_others = null;

	public Doc(String title, String ref)
	{
		m_title = title;
		if(m_title == null) m_title = "";
		m_ref = ref;
		if(m_ref == null) m_ref = "";
		m_others = new ArrayList<String>();
	}

	public void add(String text)
	{
		m_others.add(text);
	}
}
