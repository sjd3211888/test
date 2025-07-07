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
// 2024-04 ~ 2025-05: Continuous optimization of idlgen: including support for inputting idl files in directory form, resolving concurrent failure to produce files with the same name, adding validation rules for types in idl, and standardization of template files. 
// ------------------------------------------------------------------

package com.eprosima.idl.util;

import java.io.File;
import java.util.Vector;
import java.util.Deque;
import java.util.ArrayDeque;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.nio.file.Files;
import java.util.stream.Stream;
import java.io.IOException;

public class Util
{
    private static final int POLYNOMIAL = 0x1021; // Generator polynomial
    private static final int INITIAL_VALUE = 0xFFFF; // Initial value for CRC

    public static String getIDLFileNameOnly(String idlFilename)
    {
        int index = -1;
        String auxString = idlFilename, returnedValue = null;

        index = idlFilename.lastIndexOf(File.separator);

        if(index == -1)
        {
            index = idlFilename.lastIndexOf('/');
        }

        if(index != -1)
        {
            auxString = idlFilename.substring(index + 1);
        }

        // Remove '.idl'
        returnedValue = auxString.substring(0, auxString.length() - 4);

        return returnedValue;
    }

    public static Vector<String> getIDLDirFromRootDir(Vector<String> idlFilename, boolean is_not_use_recursion)
    {
        Deque<String> deque = new ArrayDeque<>(idlFilename);
        int size_count = deque.size();
        for (int count = 0; count < size_count; ++count) {
            String tmp_idl_path = deque.removeFirst();
            File file = new File(tmp_idl_path);
            if (file.exists() && file.isDirectory()) {
                Path startPath = Paths.get(tmp_idl_path);
                try (Stream<Path> stream = (is_not_use_recursion ? Files.list(startPath) : Files.walk(startPath))) {
                    stream
                        .filter(Files::isRegularFile)
                        .filter(path -> path.toString().endsWith(".idl"))
                        .map(Path::toString)
                        .forEach(deque::addLast);
                } catch (IOException e) {
                    System.err.println("An IOException occurred: " + e.getMessage());
                }
            } else if (file.exists()) {
                deque.addLast(tmp_idl_path);
            } else {
                System.out.println("The path is not exist. " + tmp_idl_path);
            }
        }
        Vector<String> vector = new Vector<>(deque);
        return vector;
    }

    public static String getIDLFileOnly(String idlFileURL) {
        String idlFileURLTmp = idlFileURL;
        int index = -1;
        String returnedValue = null;

        if (idlFileURLTmp.length() > 1 && idlFileURLTmp.charAt(1) == ':') {
            idlFileURLTmp = idlFileURLTmp.substring(2);
        }

        return idlFileURLTmp.replace(File.separatorChar, '_').replace('/', '_').replace('\\', '_');
    }

    public static String getIDLFileDirectoryOnly(String idlFileURL)
    {
        int index = -1;
        String returnedValue = null;

        index = idlFileURL.lastIndexOf(File.separator);

        if(index == -1)
            index = idlFileURL.lastIndexOf('/');

        if(index != -1)
            returnedValue = idlFileURL.substring(0, index + 1);

        return returnedValue;
    }

    public static String stringTrimAll(String str)
    {
        String trimstr = str.replaceAll("\\s+", "").toUpperCase();
        return trimstr;
    }

    public static String stringHashToHex(String input) {
        byte[] data = input.getBytes();
        int crc = INITIAL_VALUE;

        for (byte b : data) {
            crc ^= (b << 8);
            for (int i = 0; i < 8; i++) {
                if ((crc & 0x8000) != 0) {
                    crc = (crc << 1) ^ POLYNOMIAL;
                } else {
                    crc <<= 1;
                }
            }
            crc &= 0xFFFF; // Mask to 16 bits
        }

        // Convert the CRC value to a 4-digit hexadecimal string
        return String.format("%04X", crc);
    }
}
