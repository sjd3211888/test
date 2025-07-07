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

package com.eprosima.vbsrpc.wadl.util;

import com.javadude.antxr.RecognitionException;

public class TypeConverter
{
	private static final String BOOLEAN_TRUE = "true";
	private static final String BOOLEAN_FALSE = "false";

	public static boolean stringToBoolean(String string) throws RecognitionException
	{
		if(string.equals(BOOLEAN_TRUE))
			return true;
		if(string.equals(BOOLEAN_FALSE))
			return false;

		throw new RecognitionException("Bad boolean type \"" + string + "\"");
	}
}
