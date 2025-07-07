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
// 2023-05 ~ 2025-05: Enhancement of idlgen and idl2xml: support pubsub mode java/jni, idl2xml parser, RTI idl import, VBS framework, tostring/get_type_name, serialization/deserialization, xcdr, -hideInternals, zero copy, data structure optimization, etc.
// ------------------------------------------------------------------

package com.eprosima.vbsdds;
import com.eprosima.vbscdr.idl.generator.RustTypesGenerator;
import com.eprosima.vbscdr.idl.generator.TypesGenerator;
import com.eprosima.vbscdr.idl.generator.IDLTypesGenerator;
import com.eprosima.vbscdr.idl.generator.XMLTypesGenerator;
import com.eprosima.vbsdds.exceptions.BadArgumentException;
import com.eprosima.vbsdds.idl.grammar.Context;
import com.eprosima.vbsdds.solution.Project;
import com.eprosima.vbsdds.solution.Solution;
import com.eprosima.vbsdds.util.Utils;
import com.eprosima.vbsdds.util.VSConfiguration;
import com.eprosima.vbsrpc.vbsrpcgen;
import com.eprosima.idl.generator.manager.TemplateExtension;
import com.eprosima.idl.generator.manager.TemplateGroup;
import com.eprosima.idl.generator.manager.TemplateManager;
import com.eprosima.idl.parser.grammar.IDLLexer;
import com.eprosima.idl.parser.grammar.IDLParser;
import com.eprosima.idl.parser.tree.AnnotationDeclaration;
import com.eprosima.idl.parser.tree.AnnotationMember;
import com.eprosima.idl.parser.tree.Specification;
import com.eprosima.idl.parser.typecode.Kind;
import com.eprosima.idl.parser.typecode.PrimitiveTypeCode;
import com.eprosima.idl.parser.typecode.TypeCode;
import com.eprosima.idl.util.Util;
import com.eprosima.log.ColorMessage;
import com.eprosima.vbsdds.util.ValueHolder;
import java.io.BufferedReader;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.util.ArrayList;
import java.util.List;
import java.util.Vector;
import java.util.regex.Pattern;
import org.antlr.stringtemplate.StringTemplate;
import org.antlr.stringtemplate.StringTemplateErrorListener;
import org.antlr.stringtemplate.StringTemplateGroup;
import org.antlr.stringtemplate.language.DefaultTemplateLexer;
import org.antlr.v4.runtime.ANTLRFileStream;
import org.antlr.v4.runtime.CommonTokenStream;
import java.io.*;
import java.util.Arrays;
import java.util.List;
import java.util.Map;
import java.util.HashMap;

// TODO: Implement Solution & Project in com.eprosima.vbsdds.solution

public class vbsddsgen {

    /*
     * -----------------------------------------------------------------------------
     * -----------
     *
     * Attributes
     */

    private static ArrayList<String> m_platforms = null;

    private Vector<String> m_idlFiles;
    protected static String m_appEnv = "VBSRTPSHOME";
    private String m_onlyFileName = "";
    private String m_firstFileName = "";
    private String m_exampleOption = null;
    private String m_typeOption = null;
    private boolean m_ppDisable = false; // TODO
    private boolean m_replace = false;
    private boolean m_useString = false;
    private String m_ppPath = null;
    private ArrayList<String> m_xmlString = new ArrayList<String>();
    private final String m_defaultOutputDir = "." + File.separator;
    private String m_outputDir = m_defaultOutputDir;
    private String m_rustoutputDir = m_defaultOutputDir;
    private String m_tempDir = null;
    protected static String m_appName = "idlgen";

    private boolean m_publishercode = true;
    private boolean m_subscribercode = true;
    private boolean m_atLeastOneStructure = false;
    private boolean m_hasMutable = false;
    private boolean m_dynamic_end_point = false;
    private boolean m_not_use_random = false;
    private boolean m_not_use_recursion = false;
    private boolean m_gen_both_for_rti = false;

    // Enable heap management for very large data structures
    private boolean m_use_heap_allocation_strategy = false;

    // Toggle to enable hiding implementation details
    private boolean m_hide_internals = false;

    // The switch is used to control whether to enable pure data structure mode
    private boolean m_pure_structure = false;

    private int m_size_upper_limit = 80000;
    private boolean m_use_static_capacity_seq = false;
    private int m_static_seq_capacity = 80000;

    private boolean m_test_crc = false;
    private boolean m_use_debug_info = false;
    private boolean m_use_estimate_size = false;
    private boolean m_use_pack_one = false;

    protected static String m_localAppProduct = "vbsrtps";
    private ArrayList<String> m_includePaths = new ArrayList<String>();

    private static VSConfiguration m_vsconfigurations[] = {
            new VSConfiguration("Debug DLL", "Win32", true, true),
            new VSConfiguration("Release DLL", "Win32", false, true),
            new VSConfiguration("Debug", "Win32", true, false),
            new VSConfiguration("Release", "Win32", false, false)
    };

    private String m_os = null;
    private boolean fusion_ = false;

    // Generates type naming compatible with ROS 2
    private boolean m_type_ros2 = false;

    // Is RTI idl type
    private boolean m_rti_idl = false;

    // Generate TypeObject files?
    private boolean m_type_object_files = false;

    // Generate string and sequence types compatible with C?
    private boolean m_typesc = false;

    // Generate python binding files
    private boolean m_python = false;

    private boolean m_case_sensitive = false;

    // Whether to produce header files with hpp suffix
    private boolean m_gen_hpp_file = false;

    private boolean m_use_static_xml = false;

    // Whether to produce test example file
    private boolean m_test_example = false;

    // Whether has extra namespace
    private String m_extra_namespace = "";

    // Testing
    private boolean m_test = false;

    // Use type alias
    private boolean m_use_type_alias = false;

    // Use dynamic length string
    private boolean m_use_dynamic_length_string = false;

    // Use vbs framework mode
    private boolean m_use_vbs_framework = false;

    // Use dds zero copy
    private boolean m_use_zero_copy = false;

    // dds zero copy size limit
    private int m_size_limit = 0;

    // Generate shared lib
    private boolean m_gen_shared_lib = false;

    private boolean m_first_generated = false;

    private boolean m_not_use_print_help = false;

    private boolean m_not_use_global_namespace = false;

    private boolean m_use_big_alignment_type = false;
    // Use to know the programming language

    public interface StringConvertible {
        String toStringValue();
    }

    public enum LANGUAGE implements StringConvertible {
        CPP,
        JAVA,
        C,
        CJ,
        XML,
        RUST,
        RTIIDL;

        @Override
        public String toStringValue() {
            return this.name();
        }
    };
    //Used for singel line out
    private boolean m_singleLineOutput=false;

    //Used for all idl output one xml
    private boolean m_outOneXml=false;
    public enum ExampleType {
        Static,
        Static22,
        Dynamic,
        Dynamic22,
        ZeroCopy,
        BigBuffer,
        DataToJson
    };

    public ArrayList<String> getXmlArray() {
        return m_xmlString;
    }

    private static List<String> callPythonScript(String scriptPath, String idlFilePath) {
        List<String> topStructs = new ArrayList<>();
        ProcessBuilder processBuilder = new ProcessBuilder("python3", scriptPath, idlFilePath);
        try {
            Process process = processBuilder.start();

            // Read the script's output, which includes the top-level structures
            BufferedReader reader = new BufferedReader(new InputStreamReader(process.getInputStream()));
            String line;
            while ((line = reader.readLine()) != null) {
                topStructs.add(line);
            }

            // Wait for the process to finish and get the exit code
            int exitCode = process.waitFor();
            if (exitCode != 0) {
                throw new RuntimeException("Python script exited with error code: " + exitCode);
            }
        } catch (IOException e) {
            throw new RuntimeException("Error running Python script.", e);
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
            throw new RuntimeException("Python script execution interrupted.", e);
        }

        return topStructs;
    }

    private void updatePackage() {
        if (m_languageOption == LANGUAGE.RUST) {
            m_package = "types";
        } else {
            m_package = "javaDefault";
        }
    }

    private LANGUAGE m_languageOption = LANGUAGE.CPP; // Default language -> CPP

    // ! Default package used in Rust/Java files.
    private String m_package = "javaDefault";

    private ExampleType m_exampleType = ExampleType.Dynamic; // Default ExampleType -> Dynamic

    private String m_xmlType = "UDP";

    /*
     * -----------------------------------------------------------------------------
     * -----------
     *
     * Constructor
     */

    public vbsddsgen(
            String[] args) throws BadArgumentException {

        int count = 0;
        String arg;

        // Detect OS
        m_os = System.getProperty("os.name");

        m_idlFiles = new Vector<String>();

        // Check arguments
        while (count < args.length) {

            arg = args[count++];

            if (!arg.startsWith("-")) {
                m_idlFiles.add(arg);
            } else if (arg.equals("-type")) {
                if (count < args.length) {
                    m_typeOption = args[count++];
                    // TODO: add argument judgment
                } else {
                    throw new BadArgumentException("No architecture speficied after -type argument");
                }
            } else if (arg.equals("-example")) {
                if (count < args.length) {
                    m_exampleOption = args[count++];
                    if (!m_platforms.contains(m_exampleOption)) {
                        throw new BadArgumentException("Unknown example arch " + m_exampleOption);
                    }
                } else {
                    throw new BadArgumentException("No architecture speficied after -example argument");
                }
            } else if (arg.equals("-language")) {
                if (count < args.length) {
                    String languageOption = args[count++];

                    if (languageOption.equalsIgnoreCase("c++")) {
                        m_languageOption = LANGUAGE.CPP;
                    } else if (languageOption.equalsIgnoreCase("java")) {
                        m_languageOption = LANGUAGE.JAVA;
                    } else if (languageOption.equalsIgnoreCase("CJ")) {
                        m_languageOption = LANGUAGE.CJ;
                    } else if (languageOption.equalsIgnoreCase("XML")) {
                        m_languageOption = LANGUAGE.XML;
                    } else if (languageOption.equalsIgnoreCase("RUST")) {
                        m_languageOption = LANGUAGE.RUST;
                    } else if (languageOption.equalsIgnoreCase("RTIIDL")) {
                        m_languageOption = LANGUAGE.RTIIDL;
                    } else {
                        throw new BadArgumentException("Unknown language " + languageOption);
                    }
                    updatePackage();
                } else {
                    throw new BadArgumentException("No language specified after -language argument");
                }
            } else if (arg.equals("-package")) {
                if (count < args.length) {
                    m_package = args[count++];
                } else {
                    throw new BadArgumentException("No package after -package argument");
                }
            } else if (arg.equals("-ppPath")) {
                if (count < args.length) {
                    m_ppPath = args[count++];
                } else {
                    throw new BadArgumentException("No URL specified after -ppPath argument");
                }
            } else if (arg.equals("-ppDisable")) {
                m_ppDisable = true;
            } else if (arg.equals("-replace")) {
                m_replace = true;
            } else if (arg.equals("-useString")) {
                m_useString = true;
            } else if (arg.equals("-d")) {
                if (count < args.length) {
                    m_outputDir = Utils.addFileSeparator(args[count++]);
                } else {
                    throw new BadArgumentException("No URL specified after -d argument");
                }
            } else if (arg.equals("-rustdir")) {
                if (count < args.length) {
                    m_rustoutputDir = Utils.addFileSeparator(args[count++]);
                } else {
                    throw new BadArgumentException("No URL specified after -d argument");
                }
            } else if (arg.equals("-t")) {
                if (count < args.length) {
                    m_tempDir = Utils.addFileSeparator(args[count++]);
                } else {
                    throw new BadArgumentException("No temporary directory specified after -t argument");
                }
            } else if (arg.equals("-version")) {
                showVersion();
                System.exit(0);
            } else if (arg.equals("-help")) {
                printHelp();
                System.exit(0);
            } else if (arg.equals("-fusion")) {
                fusion_ = true;
            } else if (arg.equals("-rti")) {
                m_rti_idl = true;
            } else if (arg.equals("-typeros2")) {
                m_type_ros2 = true;
            } else if (arg.equals("-typeobject")) {
                m_type_object_files = true;
            } else if (arg.equals("-typesc")) {
                m_typesc = true;
            } else if (arg.equals("-python")) {
                m_python = true;
            } else if (arg.equals("-test")) {
                m_test = true;
            } else if (arg.equals("-typeAlias")) {
                m_use_type_alias = true;
            } else if (arg.equals("-dynamicLengthString")) {
                m_use_dynamic_length_string = true;
            } else if (arg.equals("-vbs")) {
                m_use_vbs_framework = true;
            } else if (arg.equals("-zerocopy")) {
                if (count < args.length) {
                    m_use_zero_copy = true;
                    String sizeLimit = args[count++];

                    if (Pattern.matches("\\d+", sizeLimit)) {
                        m_size_limit = Integer.parseInt(sizeLimit);
                    } else {
                        throw new BadArgumentException("Unknown size " + sizeLimit);
                    }
                } else {
                    throw new BadArgumentException("No size limit");
                }
            } else if (arg.equals("-sharedLib")) {
                m_gen_shared_lib = true;
            } else if (arg.equals("-I")) {
                if (count < args.length) {
                    m_includePaths.add("-I".concat(args[count++]));
                } else {
                    throw new BadArgumentException("No include directory specified after -I argument");
                }
            } else if (arg.equals("-cs")) {
                m_case_sensitive = true;
            } else if (arg.equals("-genhpp")) {
                m_gen_hpp_file = true;
            } else if (arg.equals("-testExample")) {
                if (count < args.length) {
                    m_test_example = true;
                    String exampleType = args[count++];

                    if(exampleType.equals("Dynamic")) {
                        m_exampleType = ExampleType.Dynamic;
                    } else if(exampleType.equals("Dynamic22")){
                        m_exampleType = ExampleType.Dynamic22;
                    } else if (exampleType.equals("Static")) {
                        m_exampleType = ExampleType.Static;
                    } else if (exampleType.equals("Static22")) {
                        m_exampleType = ExampleType.Static22;
                    } else if (exampleType.equals("ZeroCopy")) {
                        m_exampleType = ExampleType.ZeroCopy;
                    } else if (exampleType.equals("BigBuffer")) {
                        m_exampleType = ExampleType.BigBuffer;
                    } else if (exampleType.equals("DataToJson")) {
                        m_exampleType = ExampleType.DataToJson;
                    } else {
                        m_exampleType = ExampleType.Static;
                    }
                } else {
                    throw new BadArgumentException("No size limit");
                }
            } else if (arg.equals("-rmPrintHelp")) {
                m_not_use_print_help = true;
            } else if (arg.equals("-rmGlobalNP")) {
                m_not_use_global_namespace = true;
            } else if(arg.equals("-singleLineOutput")) {
                m_singleLineOutput = true;
            } else if(arg.equals("-outOneXml")) {
                m_outOneXml = true;
            } else if(arg.equals("-bigAlignment")) {
                m_use_big_alignment_type = true;
            } else if (arg.equals("-enableExtensions")) {
                m_hasMutable = true;
            } else if (arg.equals("-xmltype")) {
                if (count < args.length) {
                    m_test_example = true;
                    String exampleType = args[count++];

                    if(exampleType.equals("SHM")) {
                        m_xmlType = "SHM";
                    } else if(exampleType.equals("UDP")){
                        m_xmlType = "UDP";
                    }
                } else {
                    throw new BadArgumentException("No size limit");
                }
            } else if (arg.equals("-dynamicEndPoint")) {
                m_dynamic_end_point = true;
            } else if (arg.equals("-notUseRandomNumber")) {
                m_not_use_random = true;
            } else if (arg.equals("-notUseRecursionFind")) {
                m_not_use_recursion = true;
            } else if (arg.equals("-genBoth")) {
                m_gen_both_for_rti = true;
            } else if (arg.equals("-HeapAllocationStrategy")) {
                m_use_heap_allocation_strategy = true;
                if (count < args.length) {
                    String sizeLimit = args[count++];

                    if (Pattern.matches("\\d+", sizeLimit)) {
                        m_size_upper_limit = Integer.parseInt(sizeLimit);
                    } else {
                        count--;
                    }
                }
            } else if (arg.equals("-genExtraNamespace")) {
                if (count < args.length) {
                    m_extra_namespace = args[count++];
                } else {
                    throw new BadArgumentException("No extra namespace");
                }
            } else if (arg.equals("-StaticCapacitySeq")) {
                m_use_static_capacity_seq = true;
                if (count < args.length) {
                    String sizeLimit = args[count++];

                    if (Pattern.matches("\\d+", sizeLimit)) {
                        m_static_seq_capacity = Integer.parseInt(sizeLimit);
                    } else {
                        count--;
                    }
                }
            } else if (arg.equals("-hideInternals")) {
                m_hide_internals = true;
            } else if (arg.equals("-pureStructure")) {
                m_pure_structure = true;
            } else if (arg.equals("-testCrc")) {
                m_test_crc = true;
            } else if (arg.equals("-DebugInfo")) {
                m_use_debug_info = true;
            } else if (arg.equals("-UseEstimateSize")) {
                m_use_estimate_size = true;
            } else if (arg.equals("-PackOne")) {
                m_use_pack_one = true;
            } else // TODO: More options: -rpm, -debug
            {
                throw new BadArgumentException("Unknown argument " + arg);
            }
        }

        if (null != m_exampleOption && m_python) {
            throw new BadArgumentException("-example and -python currently are incompatible");
        }

        if (m_idlFiles.isEmpty()) {
            throw new BadArgumentException("No input files given");
        }

    }

    /*
     * -----------------------------------------------------------------------------
     * -----------
     *
     * Listener classes
     */

    class TemplateErrorListener implements StringTemplateErrorListener {
        public void error(
                String arg0,
                Throwable arg1) {
            System.out.println(ColorMessage.error() + arg0);
            arg1.printStackTrace();
        }

        public void warning(
                String arg0) {
            System.out.println(ColorMessage.warning() + arg0);
        }

    }

    /*
     * -----------------------------------------------------------------------------
     * -----------
     *
     * Main methods
     */

    public boolean execute() {
        if (!m_outputDir.equals(m_defaultOutputDir)) {
            File dir = new File(m_outputDir);

            if (!dir.exists()) {
                System.out.println(ColorMessage.error() + "The specified output directory does not exist");
                return false;
            }
        }

        if (!m_rustoutputDir.equals(m_defaultOutputDir)) {
            File dir = new File(m_rustoutputDir);

            if (!dir.exists()) {
                System.out.println(ColorMessage.error() + "The rust output directory doesn't exist");
                return false;
            }
        }

        boolean returnedValue = globalInit();

        if (returnedValue) {
            Solution solution = new Solution(m_languageOption, m_exampleOption,
                    getVersion(), m_publishercode, m_subscribercode);

            // Load string templates
            System.out.println("Loading templates...");
            TemplateManager.setGroupLoaderDirectories(
                        "com/eprosima/vbsdds/idl/templates:com/eprosima/vbscdr/idl/templates:com/eprosima/vbsdds/idl/templates/java:com/eprosima/vbsdds/idl/templates/cj:" +
                        "com/eprosima/vbsdds/idl/templates/cpp:com/eprosima/vbsdds/idl/templates/python:com/eprosima/vbsdds/idl/templates/rust:com/eprosima/vbsdds/idl/templates/xml:" +
                        "com/eprosima/vbscdr/idl/templates/struct:com/eprosima/vbscdr/idl/templates/test_templates:com/eprosima/vbsdds/idl/templates/idl:com/eprosima/vbsdds/idl/templates/common");

            // In local for all products
            if (m_os.contains("Windows")) {
                solution.addInclude("$(" + m_appEnv + ")/include");
                solution.addLibraryPath("$(" + m_appEnv + ")/lib");
                if (m_exampleOption != null) {
                    solution.addLibraryPath("$(" + m_appEnv + ")/lib/" + m_exampleOption);
                    solution.addLibraryPath("$(" + m_appEnv + ")/lib/" + m_exampleOption + "/VC/static");
                }
            }

            // If Java, include jni headers
            if (m_languageOption == LANGUAGE.JAVA || m_languageOption == LANGUAGE.CJ) {
                solution.addInclude("$(JAVA_HOME)/include");

                if (m_exampleOption != null && m_exampleOption.contains("Linux")) {
                    solution.addInclude("$(JAVA_HOME)/include/linux");
                }
            }

            if ((m_exampleOption != null || m_test) && !m_exampleOption.contains("Win")) {
                solution.addLibrary("ecdr");
            }

            // Add product library
            solution.addLibrary("ertps");

            ArrayList<String> includedIDL = new ArrayList<String>();
            m_idlFiles = Util.getIDLDirFromRootDir(m_idlFiles, m_not_use_recursion);
            for (int count = 0; returnedValue && (count < m_idlFiles.size()); ++count) {

                int idlFlag = (count == m_idlFiles.size() - 1 ? -1 : count);
                Project project = process(m_idlFiles.get(count), m_useString, idlFlag);

                if (project != null && m_useString) {
                    m_xmlString.add(project.getXml());
                }
                if (project != null) {
                    System.out.println("Adding project: " + project.getFile());
                    if (!solution.existsProject(project.getFile())) {
                        solution.addProject(project);
                    }
                } else {
                    return false;
                }

                for (String include : project.getIDLIncludeFiles()) {
                    // System.out.println(ColorMessage.error() + m_idlFiles.get(count) + " includes
                    // " + include);
                    includedIDL.add(include);
                }
            }
            // Add include idl files
            if (!m_useString && !m_outOneXml) {
                for (String included : includedIDL) {
                        Project inner = process(included, m_useString, 0);
                        if (inner != null && !solution.existsProject(inner.getFile())) {
                            System.out.println("Adding project: " + inner.getFile());
                            solution.addProject(inner);
                        }
                }
            }

            if (returnedValue && m_python) {
                returnedValue = genSwigCMake(solution);
            }

            // Generate solution
            if (returnedValue && (m_exampleOption != null) || m_test) {
                if ((returnedValue = genSolution(solution)) == false) {
                    System.out.println(ColorMessage.error() + "While the solution was being generated");
                }
            }

        }

        return returnedValue;

    }

    /*
     * -----------------------------------------------------------------------------
     * -----------
     *
     * Auxiliary methods
     */

    public static boolean loadPlatforms() {

        boolean returnedValue = false;

        vbsddsgen.m_platforms = new ArrayList<String>();

        vbsddsgen.m_platforms.add("i86Win32VS2019");
        vbsddsgen.m_platforms.add("x64Win64VS2019");
        vbsddsgen.m_platforms.add("i86Linux2.6gcc");
        vbsddsgen.m_platforms.add("x64Linux2.6gcc");
        vbsddsgen.m_platforms.add("armLinux2.6gcc");
        vbsddsgen.m_platforms.add("CMake");
        vbsddsgen.m_platforms.add("Cargo");

        returnedValue = true;

        return returnedValue;
    }

    private String getVersion() {
        try {
            // InputStream input =
            // this.getClass().getResourceAsStream("/vbsrtps_version.h");

            InputStream input = this.getClass().getClassLoader().getResourceAsStream("version");
            byte[] b = new byte[input.available()];
            input.read(b);
            String text = new String(b);
            int beginindex = text.indexOf("=");
            return text.substring(beginindex + 1);
        } catch (Exception ex) {
            System.out.println(ColorMessage.error() + "Getting version. " + ex.getMessage());
        }

        return "";
    }

    private void showVersion() {
        String version = getVersion();
        System.out.println(m_appName + " version " + version);
    }

    public static void printHelp() {
        System.out.println(m_appName + " usage:");
        System.out.println("\t" + m_appName + " [options] <file> [<file> ...]");
        System.out.println("\t\t-help: shows this help");
        System.out.println("\t\t-language <C++/C/java/rust/cj/xml/rtiidl>: Programming language (default: C++).");
        System.out.println("\t\t\t*C:Generate C code using VBS GEN.");
        System.out.println("\t\t\t*C++:Generate C++ code using VBS GEN.");
        System.out.println("\t\t\t*java:Generate Java code using VBS GEN.");
        System.out.println("\t\t\t*rust:Generate Rust code using VBS GEN.");

        System.out.println("\t-type <type>:Choose whether to generate DDS code or RPC code.");
        System.out.println("\t\tSupported type:");
        System.out.println("\t\t*DDS");
        System.out.println("\t\t*RPC");
        System.out.println("");

        System.out.println("\tThe DDS options are as follows:");
        System.out.println("\t\t-version: shows the current version of eProsima VBS DDS gen.");
        System.out.println(
                "\t\t-example <platform>: Generates a solution for a specific platform (example: x64Win64VS2019)");
        System.out.println("\t\t\tSupported platforms:");
        for (int count = 0; count < m_platforms.size(); ++count) {
            System.out.println("\t\t\t * " + m_platforms.get(count));
        }
        // System.out.println("\t\t-language <C++>: Programming language (default:
        // C++).");
        System.out.println("\t\t-replace: replaces existing generated files.");
        System.out.println("\t\t-typeAlias: change pusubtype to type.");
        System.out.println("\t\t-dynamicLengthString: use dynamic length string.");
        System.out.println("\t\t-vbs: use vbs framework interface");
        System.out.println("\t\t-zerocopy [cap_size]: use big buffer zerocopy mode and the cap_size is the big buffer threshold");
        System.out.println("\t\t-rti: Use RTI idl.");
        System.out.println("\t\t-HeapAllocationStrategy: Enabling the -HeapAllocationStrategy allows the use of an extensive heap management feature. You can specify a number to set the stack management limit for members. If this limit is exceeded, the allocation will be managed on the heap. By default, this size is set to 80KB.");
        System.out.println("\t\t-ppDisable: disables the preprocessor.");
        System.out.println("\t\t-ppPath: specifies the preprocessor path.");
        System.out.println("\t\t-enableExtensions: Whether to enable xcdr to use @mutable, @final, @appendable or @optional.");
        System.out.println("\t\t-hideInternals: Toggle to enable hiding implementation details.");
        System.out.println("\t\t-testExample <Dynamic/Dynamic22/Static/Static22/ZeroCopy/BigBuffer>: For producing test programs with direct communication.");
        System.out.println("\t\t-xmltype <UDP/SHM>: Used in conjunction with -testExample to produce xml of different communication types.");
        System.out.println("\t\t-dynamicEndPoint: Used with -testExample to produce sub as dynamically typed code.");
        System.out.println("\t\t-notUseRandomNumber: If enabled, header file macro protection will no longer include random numbers.");
        System.out.println("\t\t-notUseRecursionFind: If enabled, idl directory import does not support recursive traversal of subdirectories.");
        System.out.println("\t\t-typeros2: generates type naming compatible with ROS2.");
        System.out.println("\t\t-genExtraNamespace: If enabled, the namespace name following this pre-option will be injected into the dds cpp product.");
        System.out.println("\t\t-genBoth: If enabled, the data structure produced will be suitable for both DDS and RPC.");
        System.out.println("\t\t-pureStructure: If enabled, the switch is used to control whether to enable pure data structure mode.");
        System.out.println("\t\t-I <path>: add directory to preprocessor include paths.");
        System.out.println("\t\t-d <path>: sets an output directory for generated files.");
        System.out.println("\t\t-rustdir <path>: sets an output directory for generated rust files.");
        System.out.println("\t\t-t <temp dir>: sets a specific directory as a temporary directory.");
        System.out.print("\t\t-typeobject: generates TypeObject files to automatically register the types as");
        System.out.print("\t\t-StaticCapacitySeq: Enabling the -StaticCapacitySeq option activates the use of a static capacity sequence feature within the program. You can specify a numeric value to set the capacity limit for sequences. If a sequence exceeds this limit, it will be managed differently. By default, this capacity is set to 80,000 units.");
        System.out.print("\t\t-testCrc: Enables internal testing of CRC (Cyclic Redundancy Check) logic, used for validating data integrity during development.");
        System.out.print("\t\t-DebugInfo: Enables debug level logging; defaults to info level if not set.");
        System.out.print("\t\t-UseEstimateSize: In C++ production, this option adjusts the return value of getMaxCdrSerializedSize to align strictly with sizeof style size calculations, ensuring more accurate memory allocation estimations.");
        System.out.print("\t\t-PackOne: Enable 1-byte alignment for all generated data structures. When this option is on, all struct/class fields will be packed without padding, ensuring memory-tight layouts (may affect interoperability).");
        System.out.println(" dynamic.");
        System.out.println("\t\t-cs: IDL grammar apply case sensitive matching.");
        System.out.println("\t\t-genhpp: support generate hpp head file");
        System.out.println("\t\t-singleLineOutput: output content on one line");
        System.out.println("\t\t-outOneXml: multiple idl output single xml");

        System.out.println("\t\t-test: executes VBSDDSGen tests.");
        System.out.println("\t\t-python: generates python bindings for the generated types.");
        System.out.println("\tand the supported input files are:");
        System.out.println("\t* IDL files.");
        System.out.println("");

        // RPC help
        System.out.println("\tThe RPC options are as follows:");
        System.out.println("\t\t-version: shows the current version of eProsima RPC");
        System.out.println(
                "\t\t-example <platform>: Generates a solution for a specific platform (example: x64Win64VS2010)");
        System.out.println("\t\t\tSupported platforms:");
        System.out.println("\t\t\t * i86Win32VS2013");
        System.out.println("\t\t\t * x64Win64VS2013");
        System.out.println("\t\t\t * i86Win32VS2015");
        System.out.println("\t\t\t * x64Win64VS2015");
        System.out.println("\t\t\t * i86Linux2.6gcc4.4.5");
        System.out.println("\t\t\t * x64Linux2.6gcc4.4.5");
        System.out.println("\t\t\t * CMake");
        System.out.println("\t\t\t * Cargo");

        System.out.println("\t\t-stream <stream>: Generates source code for a stream mode (default: standard)");

        // " -language <C++>: Programming language (default: C++).\n" +
        System.out.println("\t\t-replace: replaces existing generated files.");
        System.out.println("\t\t-d <path>: sets an output directory for generated files.");
        System.out.println("\t\t-ppPath <path\\><program> : C/C++ Preprocessor path.(Default is cl.exe)");
        System.out.println("\t\t-ppDisable               : Do not use C/C++ preprocessor.");
        System.out.println("\t\t-t <temp dir>: sets a specific directory as a temporary directory.");
        System.out.println("\tand the supported input files are:");
        System.out.println("\t* IDL files.");

        // // RTI Gen help
        // System.out.println("");
        // System.out.println("\trtiddsgen [-help]");
        // System.out.println("\t\t[-allocateWithMalloc]");
        // System.out.println("\t\t[-alwaysUseStdVector]");
        // System.out.println("\t\t[-autoGenFiles <arch>]");
        // System.out.println("\t\t[-constructor]");
        // System.out.println("\t\t[-convertToIdl | -convertToXml | -convertToXsd]");
        // System.out.println("\t\t[-corba [client header file] [-orb <CORBA C++ ORB>]]");
        // System.out.println("\t\t[-create <typefiles, examplefiles, makefiles>]");
        // System.out.println("\t\t[-D <name>[=<value>]]");
        // System.out.println("\t\t[-d <outdir>]");
        // System.out.println("\t\t[-dataReaderSuffix <Suffix>]");
        // System.out.println("\t\t[-dataWriterSuffix <Suffix>]");
        // System.out.println("\t\t[-disableXSDValidation]");
        // System.out.println("\t\t[-dllExportMacroSuffix <suffix>]");
        // System.out.println("\t\t[-enableEscapeChar]");
        // System.out.println("\t\t[-qualifiedEnumerator]");
        // System.out.println("\t\t[-example <arch>]");
        // System.out.println("\t\t[-express ]");
        // System.out.println("\t\t[-I <directory>]");
        // System.out.println("\t\t[[-inputIdl] <IDLInputFile.idl> |");
        // System.out.println("\t\t [-inputXml] <XMLInputFile.xml>] |");
        // System.out.println("\t\t [-inputXsd] <XSDInputFile.xsd>] |");
        // System.out.println("\t\t[-language <C|C++|Java|Ada|C++/CLI|C++03|C++11|C#");
        // System.out.println("\t\t             |microC|microC++>]");
        // System.out.println("\t\t[-legacyPlugin]");
        // System.out.println("\t\t[-micro]");
        // System.out.println("\t\t[-namespace]");
        // System.out.println("\t\t[-obfuscate]");
        // System.out.println("\t\t[-optimization <level of optimization>]");
        // System.out.println("\t\t[-package <packagePrefix>]");
        // System.out.println("\t\t[-platform <arch>]");
        // System.out.println("\t\t[-ppDisable]");
        // System.out.println("\t\t[-ppPath <path to the preprocessor>]");
        // System.out.println("\t\t[-ppOption <option>]");
        // System.out.println("\t\t[-reader]");
        // System.out.println("\t\t[-replace]");
        // System.out.println("\t\t[-sequenceSize <Unbounded sequences size>]");
        // System.out.println("\t\t[-sharedLib]");
        // System.out.println("\t\t[-stringSize <Unbounded strings size>]");
        // System.out.println("\t\t[-typeSequenceSuffix <Suffix>]");
        // System.out.println("\t\t[-U <name>]");
        // System.out.println("\t\t[-unboundedSupport]");
        // System.out.println("\t\t[-update <typefiles, examplefiles, makefiles>]");
        // System.out.println("\t\t[-use52Keyhash]");
        // System.out.println("\t\t[-use526Keyhash]");
        // System.out.println("\t\t[-useStdString]");
        // System.out.println("\t\t[-V <name>[=<value>]]");
        // System.out.println("\t\t[-verbosity <1-3>]");
        // System.out.println("\t\t[-version]");
        // System.out.println("\t\t[-virtualDestructor]");
        // System.out.println("\t\t[-writer]");

    }

    public boolean globalInit() {

        // Set the temporary folder
        if (m_tempDir == null) {
            if (m_os.contains("Windows")) {
                String tempPath = System.getenv("TEMP");

                if (tempPath == null) {
                    tempPath = System.getenv("TMP");
                }

                m_tempDir = tempPath;
            } else if (m_os.contains("Linux") || m_os.contains("Mac")) {
                m_tempDir = "/tmp/";
            }
        }

        if (m_tempDir.charAt(m_tempDir.length() - 1) != File.separatorChar) {
            m_tempDir += File.separator;
        }

        return true;
    }

    private Project process(
            String idlFilename, boolean useString, int isLast) {
        Project project = null;
        System.out.println("Processing the file " + idlFilename + "...");

        try {
            // Protocol CDR
            project = parseIDL(idlFilename, useString, isLast); // TODO: Quitar archivos copiados TypesHeader.stg, TypesSource.stg,
                                             // PubSubTypeHeader.stg de la carpeta com.eprosima.vbsdds.idl.templates
        } catch (Exception ioe) {
            System.out.println(ColorMessage.error() + "Cannot generate the files");
            if (!ioe.getMessage().equals("")) {
                System.out.println(ioe.getMessage());
                ioe.printStackTrace();
            }
        }

        return project;

    }

    public List<String> getStructListV2(String idlFile) {
        Map<String, Integer> structInfoDict = new HashMap<>();
        List<String> blockStrList = new ArrayList<>();
        List<String> topStruct = new ArrayList<>();

        try (BufferedReader reader = new BufferedReader(new FileReader(idlFile))) {
            String lineContent;
            StringBuilder blockContent = new StringBuilder();
            int blockIndex = 0;
            boolean inStruct = false;

            while ((lineContent = reader.readLine()) != null) {
                lineContent = lineContent.trim();

                // Check for start of a struct
                if (lineContent.startsWith("struct ")) {
                    inStruct = true;
                }

                // Collect block content if inside a struct
                if (inStruct) {
                    blockContent.append(lineContent).append(" ");
                }

                // Check for end of a struct
                if (lineContent.endsWith("};") && inStruct) {
                    blockStrList.add(blockContent.toString());

                    int head = blockContent.indexOf("struct");
                    int tail = blockContent.indexOf("{");

                    if (head != -1 && tail != -1) {
                        String substr = blockContent.substring(head + 7, tail).trim();
                        String structName = substr.split(" ")[0];
                        structInfoDict.put(structName, blockIndex);
                        blockIndex++;
                    }

                    blockContent = new StringBuilder();
                    inStruct = false;
                }
            }

            int length = blockStrList.size();
            for (Map.Entry<String, Integer> entry : structInfoDict.entrySet()) {
                String name = entry.getKey();
                int index = entry.getValue();
                boolean flag = false;

                for (int i = 0; i < length; i++) {
                    if (i != index && blockStrList.get(i).contains(name)) {
                        flag = true;
                        break;
                    }
                }

                if (!flag) {
                    topStruct.add(name);
                }
            }
        } catch (IOException e) {
            e.printStackTrace();
        }

        return topStruct;
    }

    private Project parseIDL(
            String idlFilename, boolean useString,
            int isLast) {
        boolean returnedValue = false;
        String idlParseFileName = idlFilename;
        Project project = null;
        String onlyFileName = Util.getIDLFileNameOnly(idlFilename);
        m_onlyFileName = onlyFileName;
        if (m_firstFileName.isEmpty()) {
            m_firstFileName = onlyFileName;
        }
        if (!m_ppDisable) {
            idlParseFileName = callPreprocessor(idlFilename);
        }
        if (idlParseFileName != null) {
            Context ctx = new Context(onlyFileName, idlFilename, m_includePaths, m_subscribercode, m_publishercode,
                    m_localAppProduct, m_type_object_files, m_typesc, m_type_ros2);

            ctx.setTypeAliasFlag(m_use_type_alias);
            ctx.setGenBoth(m_gen_both_for_rti);
            ctx.setHeapAllocation(m_use_heap_allocation_strategy);
            ctx.setSizeUpperLimit(m_size_upper_limit);
            ctx.setRtiIdlFlag(m_rti_idl);
            ctx.setSingleLineOutputFlag(m_singleLineOutput);
            ctx.setDynamicLengthStringFlag(m_use_dynamic_length_string);
            ctx.setUseVbsFrameworkFlag(m_use_vbs_framework);
            ctx.setMaxBufferSize(m_size_limit);
            ctx.setTestExampleFlag(m_test_example);
            ctx.setOnlyFileName(m_firstFileName);
            ctx.setRmPrintHelper(m_not_use_print_help);
            ctx.setRmGlobalNP(m_not_use_global_namespace);
            ctx.setBigAlignment(m_use_big_alignment_type);
            ctx.setHasMutable(m_hasMutable);
            ctx.setUseUDP(m_xmlType.equals("UDP"));
            ctx.setUseDebugLog(m_use_debug_info);
            ctx.setDynamicEndPointFlag(m_dynamic_end_point);
            ctx.setNotUseRandom(m_not_use_random);
            ctx.setExtraNamespace(m_extra_namespace);
            ctx.setPureStructure(m_pure_structure & (m_languageOption == LANGUAGE.CPP));
            ctx.setTestCrc(m_test_crc);
            ctx.setUseEstimateSize(m_use_estimate_size);
            ctx.setUsePackOne(m_use_pack_one);
            if (m_use_static_capacity_seq) {
                ctx.setStaticCapacity(m_static_seq_capacity);
            }
            ctx.setHideInternals(m_hide_internals);
            if(m_exampleType == ExampleType.Dynamic22 || m_exampleType == ExampleType.Static22) {
                ctx.setIsMultiFlag(true);
            }
            if (m_exampleType == ExampleType.ZeroCopy) {
                ctx.setNormalZeroCopyFlag(true);
            }
            ctx.setOnlyFileName(m_firstFileName);
            if (m_languageOption == LANGUAGE.CJ)
                ctx.setCJ(true);
            if (m_case_sensitive) {
                ctx.ignore_case(false);
            }
            if (m_use_zero_copy) {
                ctx.setZeroCopyFlag(true);
                List<String> topStructs = getStructListV2(idlFilename);
                for (String struct : topStructs) {
                    ctx.setTopStructName(struct);
                }
            }
            if (m_gen_hpp_file) {
                ctx.setGenHpp(true);
            }

            if (m_gen_shared_lib) {
                ctx.setGenSharedLib(true);
            }

            if (fusion_) {
                ctx.setActivateFusion(true);
            }
            // Create default @Key annotation.
            AnnotationDeclaration keyann = ctx.createAnnotationDeclaration("Key", null);
            keyann.addMember(new AnnotationMember("value", new PrimitiveTypeCode(Kind.KIND_BOOLEAN), "true"));

            // Create default @Topic annotation.
            AnnotationDeclaration topicann = ctx.createAnnotationDeclaration("Topic", null);
            topicann.addMember(new AnnotationMember("value", new PrimitiveTypeCode(Kind.KIND_BOOLEAN), "true"));

            // Create template manager
            TemplateManager tmanager = new TemplateManager("VBSCdrCommon:eprosima:Common", ctx, m_typesc, m_languageOption.toStringValue());

            List<TemplateExtension> extensions = new ArrayList<TemplateExtension>();
            // Load common types template
            extensions.add(new TemplateExtension("struct_type", "keyFunctionHeadersStruct"));

            // If m_pure_structure, add FixedStringHeader
            if (m_pure_structure) {
                tmanager.addGroup("FixedStringHeader");
            }

            // If m_languageOption != LANGUAGE.XML
            if (m_languageOption != LANGUAGE.XML) {
                tmanager.addGroup("SafeEnumHeader", extensions);
                tmanager.addGroup("TypesHeader", extensions);

                if (ctx.isHasMutable()) {
                    tmanager.addGroup("TypesCdrAuxHeader", extensions);
                    tmanager.addGroup("TypesCdrAuxHeaderImpl", extensions);
                }

                if (m_type_object_files) {
                    tmanager.addGroup("TypeObjectHeader", extensions);
                    tmanager.addGroup("TypeObjectSource", extensions);
                }

                extensions.clear();
                extensions.add(new TemplateExtension("struct_type", "keyFunctionSourcesStruct"));
                tmanager.addGroup("TypesSource", extensions);

                if (!m_pure_structure) {
                    if (!m_test_example || m_exampleType == ExampleType.Dynamic ||
                        m_exampleType == ExampleType.Static || m_exampleType == ExampleType.BigBuffer) {

                        // Load Publisher templates
                        tmanager.addGroup("DDSPublisherHeader");
                        extensions.clear();
                        extensions.add(new TemplateExtension("struct_type", "keyFunctionSourcesStruct"));
                        tmanager.addGroup("DDSPublisherSource", extensions);

                        // Load Subscriber templates
                        tmanager.addGroup("DDSSubscriberHeader");
                        extensions.clear();
                        extensions.add(new TemplateExtension("struct_type", "keyFunctionSourcesStruct"));
                        tmanager.addGroup("DDSSubscriberSource", extensions);

                        // Load PubSubMain template
                        tmanager.addGroup("DDSPubSubMain");
                    }
                    if (m_exampleType == ExampleType.DataToJson) {
                        tmanager.addGroup("DataToJsonMain");
                    }
                }

                // Load Types common templates
                tmanager.addGroup("DDSPubSubTypeHeader");
                tmanager.addGroup("DDSPubSubTypeSource");
            }

            if (m_test) {
                // Load test template
                tmanager.addGroup("SerializationTestSource");
                tmanager.addGroup("SerializationHeader");
                tmanager.addGroup("SerializationSource");
            }

            if (m_test_example) {
                tmanager.addGroup("TestBash");
                tmanager.addGroup("ToidlStringTest");
                if (m_exampleType == ExampleType.Dynamic22 || m_exampleType == ExampleType.Static22) {
                    tmanager.addGroup("DDSPublisherHeader2to2");
                    extensions.clear();
                    extensions.add(new TemplateExtension("struct_type", "keyFunctionSourcesStruct"));
                    tmanager.addGroup("DDSPublisherSource2to2", extensions);
                    tmanager.addGroup("DDSSubscriberHeader2to2");
                    extensions.clear();
                    extensions.add(new TemplateExtension("struct_type", "keyFunctionSourcesStruct"));
                    tmanager.addGroup("DDSSubscriberSource2to2", extensions);
                    tmanager.addGroup("DDSPubSubMain2to2");
                }
                if (m_exampleType == ExampleType.ZeroCopy) {
                    tmanager.addGroup("DDSPublisherHeaderZeroCopy");
                    extensions.clear();
                    extensions.add(new TemplateExtension("struct_type", "keyFunctionSourcesStruct"));
                    tmanager.addGroup("DDSPublisherSourceZeroCopy", extensions);
                    tmanager.addGroup("DDSSubscriberHeaderZeroCopy");
                    extensions.clear();
                    extensions.add(new TemplateExtension("struct_type", "keyFunctionSourcesStruct"));
                    tmanager.addGroup("DDSSubscriberSourceZeroCopy", extensions);
                    tmanager.addGroup("DDSPubSubMain2to2");
                }
            }

            if (m_use_static_capacity_seq) {
                tmanager.addGroup("StaticCapacityDynamicArray");
            }

            if (m_use_static_xml) {
                tmanager.addGroup("PubStaticProfileXML");
                tmanager.addGroup("PubNormalProfileXML");
                tmanager.addGroup("SubNormalProfileXML");
                tmanager.addGroup("SubStaticProfileXML");
            } else {
                tmanager.addGroup("PubDynamicProfileXML");
                tmanager.addGroup("SubDynamicProfileXML");
            }

            // Add JNI sources.
            if (m_languageOption == LANGUAGE.JAVA) {
                tmanager.addGroup("JNIHeader");
                tmanager.addGroup("JNISource");
                tmanager.addGroup("JavaSource");

                // Set package in context.
                ctx.setPackage(m_package);
                TypeCode.javapackage = m_package + (m_package.isEmpty() ? "" : ".");
            }

            if (m_languageOption == LANGUAGE.CJ) {
                tmanager.addGroup("CJPubSubJNIHeader");
                tmanager.addGroup("CJPubSubJNISource");
                tmanager.addGroup("CJPubSubClass");
                tmanager.addGroup("CJPubSubJNIIHeader");
                tmanager.addGroup("CJPubSubJNIISource");
                tmanager.addGroup("TopicDataType");
                if (!m_use_vbs_framework) {
                    tmanager.addGroup("JNISeqHeader");
                    tmanager.addGroup("JNISeqSource");
                }
                // Set package in context.
                ctx.setPackage(m_package);
                TypeCode.javapackage = m_package + (m_package.isEmpty() ? "" : ".");
            }

            if (m_languageOption == LANGUAGE.RUST) {
                if (m_use_vbs_framework) {
                    tmanager.addGroup("TypesCwrapperHeader");
                    tmanager.addGroup("TypesCwrapperSource");
                    if(m_test_example) {
                    tmanager.addGroup("RustCargoToml");
                    tmanager.addGroup("Rustconfig");
                    tmanager.addGroup("RustTestBash");
                    tmanager.addGroup("RustMain");
                    tmanager.addGroup("RustPubExample");
                    tmanager.addGroup("RustSubExample");
                    }
                    ctx.setPackage(m_package);
                    TypeCode.javapackage = m_package + (m_package.isEmpty() ? "" : ".");
                }
            }

            if (m_python) {
                tmanager.addGroup("TypesSwigInterface");
                tmanager.addGroup("DDSPubSubTypeSwigInterface");
            }

            // Create main template
            TemplateGroup maintemplates = tmanager.createTemplateGroup("main");
            maintemplates.setAttribute("ctx", ctx);
            try {
                ANTLRFileStream input = new ANTLRFileStream(idlParseFileName);
                IDLLexer lexer = new IDLLexer(input);
                lexer.setContext(ctx);
                CommonTokenStream tokens = new CommonTokenStream(lexer);
                IDLParser parser = new IDLParser(tokens);
                // Pass the finelame without the extension
                Specification specification = parser.specification(ctx, tmanager, maintemplates).spec;
                returnedValue = specification != null;
            } catch (FileNotFoundException ex) {
                System.out.println(ColorMessage.error(
                        "FileNotFounException") + "The File " + idlParseFileName + " was not found.");
            } /*
               * catch (ParseException ex) {
               * System.out.println(ColorMessage.error("ParseException") + ex.getMessage());
               * }
               */
            catch (Exception ex) {
                System.out.println(ColorMessage.error("Exception") + ex.getMessage());
            }
            if (returnedValue && m_languageOption != LANGUAGE.XML &&
                    m_languageOption != LANGUAGE.RTIIDL) {
                // Create information of project for solution
                project = new Project(ctx.getFilename(), idlFilename, ctx.getDependencies());

                System.out.println("Generating Type definition files...");
                if (returnedValue = Utils.writeFile(m_outputDir + ctx.getFilename()  + (m_rti_idl ? "Base" : "") + (m_gen_hpp_file ? ".hpp" : ".h"),
                        maintemplates.getTemplate("TypesHeader"),
                        m_replace)) {
                    if (returnedValue = Utils.writeFile(m_outputDir + ctx.getFilename() + (m_rti_idl ? "Base" : "") + ".cxx",
                            maintemplates.getTemplate("TypesSource"), m_replace)) {
                        // if (!m_not_use_print_help) {
                        //     if (returnedValue = Utils.writeFile(m_outputDir + "ContainerPrintHelpers"+(m_gen_hpp_file ? ".hpp" : ".h"),
                        //         maintemplates.getTemplate("ContainerPrintHelpers"), m_replace)) {
                        //     }
                        //     if (returnedValue = Utils.writeFile(m_outputDir + "ContainerPrintHelpers.cxx",
                        //         maintemplates.getTemplate("ContainerPrintSources"), m_replace)) {
                        //     }
                        // }
                        project.addCommonIncludeFile(ctx.getFilename()  + (m_rti_idl ? "Base" : "") + (m_gen_hpp_file ? ".hpp" : ".h"));
                        project.addCommonSrcFile(ctx.getFilename()  + (m_rti_idl ? "Base" : "") + ".cxx");
                        if (returnedValue = Utils.writeFile(m_outputDir + "SafeEnum"+(m_gen_hpp_file ? ".hpp" : ".h"),
                            maintemplates.getTemplate("SafeEnumHeader"), m_replace)) {
                        }
                        if (m_use_static_capacity_seq && (returnedValue = Utils.writeFile(m_outputDir + "StaticCapacityDynamicArray"+(m_gen_hpp_file ? ".hpp" : ".h"),
                            maintemplates.getTemplate("StaticCapacityDynamicArray"), m_replace))) {
                        }
                        if (m_pure_structure && (returnedValue = Utils.writeFile(m_outputDir + "FixedString"+(m_gen_hpp_file ? ".hpp" : ".h"),
                            maintemplates.getTemplate("FixedStringHeader"), m_replace))) {
                            m_atLeastOneStructure = true;
                        }
                        if (!m_pure_structure) {
                            if (m_type_object_files) {
                                System.out.println("Generating TypeObject files...");
                                if (returnedValue = Utils.writeFile(m_outputDir + ctx.getFilename() + "TypeObject"+(m_gen_hpp_file ? ".hpp" : ".h"),
                                        maintemplates.getTemplate("TypeObjectHeader"), m_replace)) {
                                    if (returnedValue = Utils.writeFile(m_outputDir + ctx.getFilename() + "TypeObject.cxx",
                                            maintemplates.getTemplate("TypeObjectSource"), m_replace)) {
                                        project.addCommonIncludeFile(ctx.getFilename() + "TypeObject"+(m_gen_hpp_file ? ".hpp" : ".h"));
                                        project.addCommonSrcFile(ctx.getFilename() + "TypeObject.cxx");
                                    }
                                }
                            }
                            if (m_python) {
                                System.out.println("Generating Swig interface files...");
                                if (returnedValue = Utils.writeFile(m_outputDir + onlyFileName  + (m_rti_idl ? "Base" : "") + ".i",
                                        maintemplates.getTemplate("TypesSwigInterface"), m_replace)) {
                                }
                            }
                            if (m_languageOption == LANGUAGE.RUST) {
                                System.out.println("Generating RUST wrapper files...");
                                if (returnedValue = Utils.writeFile(m_outputDir + ctx.getFilename()  +"Wrapper.h",
                                        maintemplates.getTemplate("TypesCwrapperHeader"), m_replace)) {
                                            if (returnedValue = Utils.writeFile(m_outputDir + ctx.getFilename() + "Wrapper.cpp",
                                            maintemplates.getTemplate("TypesCwrapperSource"), m_replace)) {
                                        project.addCommonIncludeFile(ctx.getFilename() + "Wrapper.h");
                                        project.addCommonSrcFile(ctx.getFilename() + "Wrapper.cpp");
                                    }
                                }
                            }
                            if (ctx.isHasMutable()) {
                                if (returnedValue =
                                        Utils.writeFile(m_outputDir + ctx.getFilename() + "CdrAux" + (m_gen_hpp_file ? ".hpp" : ".h"),
                                        maintemplates.getTemplate("TypesCdrAuxHeader"), m_replace))
                                {
                                    project.addCommonIncludeFile(ctx.getFilename() + "CdrAux" + (m_gen_hpp_file ? ".hpp" : ".h"));
                                    if(returnedValue &=
                                        Utils.writeFile(m_outputDir + ctx.getFilename() + "CdrAux.ipp",
                                            maintemplates.getTemplate("TypesCdrAuxHeaderImpl"), m_replace)) {
                                    }
                                }
                            }
                        }
                    }
                }
                if (!m_first_generated && !m_pure_structure && null != m_exampleOption) {
                    if (m_use_static_xml) {
                        if(!m_first_generated)
                            m_first_generated = true;
                        if (returnedValue = Utils.writeFile(m_outputDir + "StaticPublisher.xml",
                            maintemplates.getTemplate("PubStaticProfileXML"), m_replace)) {
                        }
                        if (returnedValue = Utils.writeFile(m_outputDir + "test_xml_pub.xml",
                            maintemplates.getTemplate("PubNormalProfileXML"), m_replace)) {
                        }
                        if (returnedValue = Utils.writeFile(m_outputDir + "test_xml_sub.xml",
                            maintemplates.getTemplate("SubNormalProfileXML"), m_replace)) {
                        }
                        if (returnedValue = Utils.writeFile(m_outputDir + "StaticSubscriber.xml",
                            maintemplates.getTemplate("SubStaticProfileXML"), m_replace)) {
                        }
                    } else {
                        if(!m_first_generated)
                            m_first_generated = true;
                        String _outputDir = m_languageOption==LANGUAGE.RUST? m_rustoutputDir : m_outputDir;
                        if (returnedValue = Utils.writeFile(_outputDir + "test_xml_pub.xml",
                            maintemplates.getTemplate("PubDynamicProfileXML"), m_replace)) {
                        }
                        if (returnedValue = Utils.writeFile(_outputDir + "test_xml_sub.xml",
                            maintemplates.getTemplate("SubDynamicProfileXML"), m_replace)) {
                        }
                    }
                }
                if (m_test && !m_pure_structure) {
                    System.out.println("Generating Serialization Test file...");
                    String fileName = m_outputDir + ctx.getFilename() + "SerializationTest.cxx";
                    returnedValue = Utils.writeFile(fileName, maintemplates.getTemplate("SerializationTestSource"),
                            m_replace);

                    System.out.println("Generating Serialization Source file...");
                    String fileNameS = m_outputDir + ctx.getFilename() + "Serialization.cxx";
                    returnedValue = Utils.writeFile(fileNameS, maintemplates.getTemplate("SerializationSource"),
                            m_replace);

                    System.out.println("Generating Serialization Header file...");
                    String fileNameH = m_outputDir + ctx.getFilename() + "Serialization"+(m_gen_hpp_file ? ".hpp" : ".h");
                    returnedValue = Utils.writeFile(fileNameH, maintemplates.getTemplate("SerializationHeader"),
                            m_replace);
                }

                if (!m_pure_structure) {
                    if (returnedValue = Utils.writeFile(m_outputDir + ctx.getFilename() + (m_rti_idl ? "" : "PubSubTypes") + (m_gen_hpp_file ? ".hpp" : ".h"),
                            maintemplates.getTemplate("DDSPubSubTypeHeader"), m_replace)) {
                        if (returnedValue = Utils.writeFile(m_outputDir + ctx.getFilename() + (m_rti_idl ? "" : "PubSubTypes") + ".cxx",
                                maintemplates.getTemplate("DDSPubSubTypeSource"), m_replace)) {

                            project.addProjectIncludeFile(ctx.getFilename() + (m_rti_idl ? "" : "PubSubTypes") + (m_gen_hpp_file ? ".hpp" : ".h"));
                            project.addProjectSrcFile(ctx.getFilename() + (m_rti_idl ? "" : "PubSubTypes") + ".cxx");
                            if (m_python) {
                                System.out.println("Generating Swig interface files...");
                                returnedValue = Utils.writeFile(
                                        m_outputDir + ctx.getFilename() + (m_rti_idl ? "" : "PubSubTypes") + ".i",
                                        maintemplates.getTemplate("DDSPubSubTypeSwigInterface"), m_replace);
                            }
                        }
                    }
                }
                // rust classes.
                if (ctx.existsLastStructure() &&returnedValue && m_languageOption == LANGUAGE.RUST && m_test_example) {
                    //for server and client
                    if (returnedValue = Utils.writeFile(
                        m_rustoutputDir + "pub_example.rs",
                            maintemplates.getTemplate("RustPubExample"), m_replace)) {
                    }
                    if (returnedValue = Utils.writeFile(
                        m_rustoutputDir + "sub_example.rs",
                            maintemplates.getTemplate("RustSubExample"), m_replace)) {
                    }
                    //for cargo build
                    if (returnedValue = Utils.writeFile(
                        m_rustoutputDir + "Cargo.toml",
                    maintemplates.getTemplate("RustCargoToml"), m_replace)) {
                    }
                    String parentDir = new File(m_rustoutputDir).getParent();
                    String cargoDir = parentDir + File.separator + ".cargo";
                    File cargoDirFile = new File(cargoDir);
                    if (!cargoDirFile.exists()) {
                        cargoDirFile.mkdirs();
                    }
                    if (returnedValue = Utils.writeFile(cargoDir + File.separator +
                        "config.toml",maintemplates.getTemplate("Rustconfig"), m_replace)) {
                    }
                    //for testrust bash
                    if (returnedValue = Utils.writeFile(m_rustoutputDir + "start.sh",
                    maintemplates.getTemplate("RustTestBash"), m_replace)) {
                    }
                    if (returnedValue = Utils.writeFile(m_rustoutputDir + "main.rs",
                    maintemplates.getTemplate("RustMain"), m_replace)) {
                    }
                }
                // TODO: Uncomment following lines and create templates
                if (ctx.existsLastStructure() && m_languageOption != LANGUAGE.XML && !m_pure_structure) {
                    m_atLeastOneStructure = true;
                    project.setHasStruct(true);

                    System.out.println("Generating TopicDataTypes files...");

                    if (m_exampleOption != null && m_languageOption != LANGUAGE.CJ && m_languageOption != LANGUAGE.RUST) {
                        System.out.println("Generating Publisher files...");
                        if((!m_test_example) || m_exampleType == ExampleType.Dynamic || m_exampleType == ExampleType.Static || m_exampleType == ExampleType.BigBuffer) {
                            if (returnedValue = Utils.writeFile(m_outputDir + ctx.getFilename() + "Publisher"+(m_gen_hpp_file ? ".hpp" : ".h"),
                                    maintemplates.getTemplate("DDSPublisherHeader"), m_replace)) {
                                if (returnedValue = Utils.writeFile(m_outputDir + ctx.getFilename() + "Publisher.cxx",
                                        maintemplates.getTemplate("DDSPublisherSource"), m_replace)) {
                                    project.addProjectIncludeFile(ctx.getFilename() + "Publisher"+(m_gen_hpp_file ? ".hpp" : ".h"));
                                    project.addProjectSrcFile(ctx.getFilename() + "Publisher.cxx");
                                }
                            }
                            System.out.println("Generating Subscriber files...");
                            if (returnedValue = Utils.writeFile(m_outputDir + ctx.getFilename() + "Subscriber"+(m_gen_hpp_file ? ".hpp" : ".h"),
                                    maintemplates.getTemplate("DDSSubscriberHeader"), m_replace)) {
                                if (returnedValue = Utils.writeFile(m_outputDir + ctx.getFilename() + "Subscriber.cxx",
                                        maintemplates.getTemplate("DDSSubscriberSource"), m_replace)) {
                                    project.addProjectIncludeFile(ctx.getFilename() + "Subscriber"+(m_gen_hpp_file ? ".hpp" : ".h"));
                                    project.addProjectSrcFile(ctx.getFilename() + "Subscriber.cxx");
                                }
                            }
                            System.out.println("Generating main file...");
                            if (returnedValue = Utils.writeFile(m_outputDir + ctx.getFilename() + "PubSubMain.cxx",
                                    maintemplates.getTemplate("DDSPubSubMain"), m_replace)) {
                                project.addProjectSrcFile(ctx.getFilename() + "PubSubMain.cxx");
                            }
                        }

                        if (m_test_example && (m_exampleType == ExampleType.Dynamic22 || m_exampleType == ExampleType.Static22)) {
                            if (returnedValue = Utils.writeFile(m_outputDir + ctx.getFilename() + "Publisher"+(m_gen_hpp_file ? ".hpp" : ".h"),
                                    maintemplates.getTemplate("DDSPublisherHeader2to2"), m_replace)) {
                                if (returnedValue = Utils.writeFile(m_outputDir + ctx.getFilename() + "Publisher.cxx",
                                        maintemplates.getTemplate("DDSPublisherSource2to2"), m_replace)) {
                                    project.addProjectIncludeFile(ctx.getFilename() + "Publisher"+(m_gen_hpp_file ? ".hpp" : ".h"));
                                    project.addProjectSrcFile(ctx.getFilename() + "Publisher.cxx");
                                }
                            }
                            if (returnedValue = Utils.writeFile(m_outputDir + ctx.getFilename() + "Subscriber"+(m_gen_hpp_file ? ".hpp" : ".h"),
                                    maintemplates.getTemplate("DDSSubscriberHeader2to2"), m_replace)) {
                                if (returnedValue = Utils.writeFile(m_outputDir + ctx.getFilename() + "Subscriber.cxx",
                                        maintemplates.getTemplate("DDSSubscriberSource2to2"), m_replace)) {
                                    project.addProjectIncludeFile(ctx.getFilename() + "Subscriber"+(m_gen_hpp_file ? ".hpp" : ".h"));
                                    project.addProjectSrcFile(ctx.getFilename() + "Subscriber.cxx");
                                }
                            }
                            System.out.println("Generating main file...");
                            if (returnedValue = Utils.writeFile(m_outputDir + ctx.getFilename() + "PubSubMain.cxx",
                                    maintemplates.getTemplate("DDSPubSubMain2to2"), m_replace)) {
                                project.addProjectSrcFile(ctx.getFilename() + "PubSubMain.cxx");
                            }
                        }

                        if (m_test_example && m_exampleType == ExampleType.ZeroCopy) {
                            if (returnedValue = Utils.writeFile(m_outputDir + ctx.getFilename() + "Publisher"+(m_gen_hpp_file ? ".hpp" : ".h"),
                                    maintemplates.getTemplate("DDSPublisherHeaderZeroCopy"), m_replace)) {
                                if (returnedValue = Utils.writeFile(m_outputDir + ctx.getFilename() + "Publisher.cxx",
                                        maintemplates.getTemplate("DDSPublisherSourceZeroCopy"), m_replace)) {
                                    project.addProjectIncludeFile(ctx.getFilename() + "Publisher"+(m_gen_hpp_file ? ".hpp" : ".h"));
                                    project.addProjectSrcFile(ctx.getFilename() + "Publisher.cxx");
                                }
                            }
                            if (returnedValue = Utils.writeFile(m_outputDir + ctx.getFilename() + "Subscriber"+(m_gen_hpp_file ? ".hpp" : ".h"),
                                    maintemplates.getTemplate("DDSSubscriberHeaderZeroCopy"), m_replace)) {
                                if (returnedValue = Utils.writeFile(m_outputDir + ctx.getFilename() + "Subscriber.cxx",
                                        maintemplates.getTemplate("DDSSubscriberSourceZeroCopy"), m_replace)) {
                                    project.addProjectIncludeFile(ctx.getFilename() + "Subscriber"+(m_gen_hpp_file ? ".hpp" : ".h"));
                                    project.addProjectSrcFile(ctx.getFilename() + "Subscriber.cxx");
                                }
                            }
                            System.out.println("Generating main file...");
                            if (returnedValue = Utils.writeFile(m_outputDir + ctx.getFilename() + "PubSubMain.cxx",
                                    maintemplates.getTemplate("DDSPubSubMain2to2"), m_replace)) {
                                project.addProjectSrcFile(ctx.getFilename() + "PubSubMain.cxx");
                            }
                        }

                        if (m_exampleType == ExampleType.DataToJson) {
                            if (returnedValue = Utils.writeFile(m_outputDir + ctx.getFilename() + "PubSubMain.cxx",
                                    maintemplates.getTemplate("DataToJsonMain"), m_replace)) {
                                project.addProjectSrcFile(ctx.getFilename() + "PubSubMain.cxx");
                            }
                        }
                        if (m_test_example) {
                            if (returnedValue = Utils.writeFile(m_outputDir + "start.sh",
                                    maintemplates.getTemplate("TestBash"), m_replace)) {
                            }
                            if (isLast == -1) {
                                if (returnedValue = Utils.writeFile(m_outputDir + ctx.getFilename() + "to_idl_load.cpp",
                                        maintemplates.getTemplate("ToidlStringTest"), m_replace)) {
                                }
                            }
                        }
                    }
                }
            }

            // RUST support
            if (returnedValue && m_languageOption == LANGUAGE.RUST && !m_pure_structure) {
                String outputDir = m_outputDir;
                // Make directories from package.
                if (!m_package.isEmpty()) {
                    outputDir = m_rustoutputDir + File.separator + m_package.replace('.', File.separatorChar);
                    File dirs = new File(outputDir);

                    if (!dirs.exists()) {
                        if (!dirs.mkdirs()) {
                            System.out.println(ColorMessage.error() + "Cannot create directories for Rust packages.");
                            return null;
                        }
                    }
                }

                RustTypesGenerator typeGen = new RustTypesGenerator(tmanager, m_rustoutputDir, m_replace, m_use_vbs_framework, ctx.getFilename());
                TypeCode.javapackage = m_package + (m_package.isEmpty() ? "" : ".");
                if (!typeGen.generate(ctx, outputDir + File.separator, m_package, ctx.getFilename(), null)) {
                    System.out.println(ColorMessage.error() + "generating Rust types");
                    return null;
                }
            }
            // Java support (Java classes and JNI code)
            if (returnedValue && m_languageOption == LANGUAGE.JAVA && !m_pure_structure) {
                String outputDir = m_outputDir;
                // Make directories from package.
                if (!m_package.isEmpty()) {
                    outputDir = m_outputDir + File.separator + m_package.replace('.', File.separatorChar);
                    File dirs = new File(outputDir);

                    if (!dirs.exists()) {
                        if (!dirs.mkdirs()) {
                            System.out.println(ColorMessage.error() + "Cannot create directories for Java packages.");
                            return null;
                        }
                    }
                }

                // Java classes.
                TypesGenerator typeGen = new TypesGenerator(tmanager, m_outputDir, m_replace, false, false, m_use_vbs_framework, m_gen_hpp_file, ctx.getFilename());
                TypeCode.javapackage = m_package + (m_package.isEmpty() ? "" : ".");
                if (!typeGen.generate(ctx, outputDir + File.separator, m_package, ctx.getFilename(), null)) {
                    System.out.println(ColorMessage.error() + "generating Java types");
                    return null;
                }

                if (ctx.existsLastStructure()) {
                    System.out.println(
                            "Generando fichero " + outputDir + File.separator + ctx.getFilename() + "PubSub.java");
                    if (!Utils.writeFile(outputDir + File.separator + ctx.getFilename() + "PubSub.java",
                            maintemplates.getTemplate("JavaSource"), m_replace)) {
                        return null;
                    }

                }

                if (Utils.writeFile(m_outputDir + ctx.getFilename() + "PubSubJNII"+(m_gen_hpp_file ? ".hpp" : ".h"),
                        maintemplates.getTemplate("JNIHeader"),
                        m_replace)) {
                    project.addJniIncludeFile(ctx.getFilename() + "PubSubJNII"+(m_gen_hpp_file ? ".hpp" : ".h"));
                } else {
                    return null;
                }

                StringTemplate jnisourceTemplate = maintemplates.getTemplate("JNISource");
                if (Utils.writeFile(m_outputDir + ctx.getFilename() + "PubSubJNI.cxx", jnisourceTemplate, m_replace)) {
                    project.addJniSrcFile(ctx.getFilename() + "PubSubJNI.cxx");
                } else {
                    return null;
                }
            }

            if (returnedValue && m_languageOption == LANGUAGE.CJ && !m_pure_structure) {
                String outputDir = m_outputDir;

                // Make directories from package.
                if (!m_package.isEmpty()) {
                    outputDir = m_outputDir + File.separator + m_package.replace('.', File.separatorChar);
                    File dirs = new File(outputDir);

                    if (!dirs.exists()) {
                        if (!dirs.mkdirs()) {
                            System.out.println(ColorMessage.error() + "Cannot create directories for Java packages.");
                            return null;
                        }
                    }
                }

                // Java classes.

                TypesGenerator typeGen = new TypesGenerator(tmanager, m_outputDir, m_replace, true, false, m_use_vbs_framework, m_gen_hpp_file, ctx.getFilename());

                if (!typeGen.generate(ctx, outputDir + File.separator, m_package, ctx.getFilename(), null)) {
                    System.out.println(ColorMessage.error() + "generating CJ types");
                    return null;
                }

                if (Utils.writeFile(
                        m_outputDir + TypeCode.javapackage.replace('.', '_') + (ctx.getTargetStructScop().equals("") ? "" : ctx.getTargetStructScop() + "_")
                                    + ctx.getFilename() + "PubSubJNI"+(m_gen_hpp_file ? ".hpp" : ".h"),
                        maintemplates.getTemplate("CJPubSubJNIHeader"),
                        m_replace)) {
                    project.addJniIncludeFile(
                            TypeCode.javapackage.replace('.', '_') + (ctx.getTargetStructScop().equals("") ? "" : ctx.getTargetStructScop() + "_")
                                        + ctx.getFilename() + "PubSubJNI"+(m_gen_hpp_file ? ".hpp" : ".h"));
                } else {
                    return null;
                }

                StringTemplate jnisourceTemplate2 = maintemplates.getTemplate("CJPubSubJNISource");
                if (Utils.writeFile(
                        m_outputDir + TypeCode.javapackage.replace('.', '_') + (ctx.getTargetStructScop().equals("") ? "" : ctx.getTargetStructScop() + "_")
                                    + ctx.getFilename() + "PubSubJNI.cxx",
                        jnisourceTemplate2, m_replace)) {
                    project.addJniSrcFile(TypeCode.javapackage.replace('.', '_') + (ctx.getTargetStructScop().equals("") ? "" : ctx.getTargetStructScop() + "_")
                                + ctx.getFilename() + "PubSubJNI.cxx");
                } else {
                    return null;
                }

                StringTemplate topicDataTypeTemplate2 = maintemplates.getTemplate("TopicDataType");
                if (Utils.writeFile(
                        m_outputDir + "VBSFramework_TopicDataType.h",
                                    topicDataTypeTemplate2, m_replace)) {
                    project.addJniIncludeFile("VBSFramework_TopicDataType.h");
                } else {
                    return null;
                }

                if (!m_use_vbs_framework) {
                    if (Utils.writeFile(
                            m_outputDir + TypeCode.javapackage.replace('.', '_') + (ctx.getTargetStructScop().equals("") ? "" : ctx.getTargetStructScop() + "_")
                                        + ctx.getFilename() + "Seq"+(m_gen_hpp_file ? ".hpp" : ".h"),
                            maintemplates.getTemplate("JNISeqHeader"),
                            m_replace)) {
                        project.addJniIncludeFile(
                                TypeCode.javapackage.replace('.', '_') + (ctx.getTargetStructScop().equals("") ? "" : ctx.getTargetStructScop() + "_")
                                            + ctx.getFilename() + "Seq"+(m_gen_hpp_file ? ".hpp" : ".h"));
                    } else {
                        return null;
                    }

                    StringTemplate jnisourceTemplate4 = maintemplates.getTemplate("JNISeqSource");
                    if (Utils.writeFile(
                            m_outputDir + TypeCode.javapackage.replace('.', '_') + (ctx.getTargetStructScop().equals("") ? "" : ctx.getTargetStructScop() + "_")
                                        + ctx.getFilename() + "Seq.cxx",
                            jnisourceTemplate4,
                            m_replace)) {
                        project.addJniSrcFile(
                                TypeCode.javapackage.replace('.', '_') + (ctx.getTargetStructScop().equals("") ? "" : ctx.getTargetStructScop() + "_")
                                            + ctx.getFilename() + "Seq.cxx");
                    } else {
                        return null;
                    }
                }
            }

            // Java support (Java classes and JNI code)
            if (returnedValue && m_languageOption == LANGUAGE.XML && !m_pure_structure) {
                String outputDir = m_outputDir;
                // Java classes.
                ValueHolder holder = new ValueHolder("");
                XMLTypesGenerator typeGen = new XMLTypesGenerator(tmanager, m_outputDir, m_replace, m_outOneXml, isLast);
                TypeCode.javapackage = m_package + (m_package.isEmpty() ? "" : ".");
                if (!typeGen.generate(ctx, outputDir + File.separator, m_package, ctx.getFilename(), null, useString, holder)) {
                    System.out.println(ColorMessage.error() + "generating XML types");
                    return null;
                }
                project = new Project(ctx.getFilename(), idlFilename, ctx.getDependencies());
                if (useString) {
                    project.setXml(holder.getXmlString());
                }
            }

            if (returnedValue && m_languageOption == LANGUAGE.RTIIDL && !m_pure_structure) {
                String outputDir = m_outputDir;
                // Java classes.
                ValueHolder holder = new ValueHolder("");
                IDLTypesGenerator typeGen = new IDLTypesGenerator(tmanager, m_outputDir, m_replace);
                TypeCode.javapackage = m_package + (m_package.isEmpty() ? "" : ".");
                if (!typeGen.generate(ctx, outputDir + File.separator, m_package, ctx.getFilename(), null)) {
                    System.out.println(ColorMessage.error() + "generating RTIIDL types");
                    return null;
                }
                project = new Project(ctx.getFilename(), idlFilename, ctx.getDependencies());
                if (useString) {
                    project.setXml(holder.getXmlString());
                }
            }
        }

        return returnedValue ? project : null;
    }

    private boolean genSolution(
            Solution solution) {

        final String METHOD_NAME = "genSolution";
        boolean returnedValue = true;
        if (m_atLeastOneStructure == true) {
            if (m_exampleOption != null) {
                System.out.println("Generating solution for arch " + m_exampleOption + "...");

                if (m_exampleOption.equals("CMake") || m_test) {
                    System.out.println("Generating CMakeLists solution");
                    returnedValue = genCMakeLists(solution);
                } else if (m_exampleOption.equals("Cargo")) {
                    System.out.println("Genering Rust Cargo solution");
                    returnedValue = genCargoBuild(solution, "64");
                } else if (m_exampleOption.substring(3, 6).equals("Win")) {
                    System.out.println("Generating Windows solution");
                    if (m_exampleOption.startsWith("i86")) {
                        returnedValue = genVS(solution, null, "16", "142");
                    } else if (m_exampleOption.startsWith("x64")) {
                        for (int index = 0; index < m_vsconfigurations.length; index++) {
                            m_vsconfigurations[index].setPlatform("x64");
                        }
                        returnedValue = genVS(solution, "x64", "16", "142");
                    } else {
                        returnedValue = false;
                    }
                } else if (m_exampleOption.substring(3, 8).equals("Linux")) {
                    System.out.println("Generating makefile solution");

                    if (m_exampleOption.startsWith("i86")) {
                        returnedValue = genMakefile(solution, "-m32");
                    } else if (m_exampleOption.startsWith("x64")) {
                        returnedValue = genMakefile(solution, "-m64");
                    } else if (m_exampleOption.startsWith("arm")) {
                        returnedValue = genMakefile(solution, "");
                    } else {
                        returnedValue = false;
                    }
                }
            }
        } else {
            if (m_languageOption != LANGUAGE.XML) {
                System.out.println(
                        ColorMessage.warning() +
                                "No structure found in any of the provided IDL; no example files have been generated");
            }
        }

        return returnedValue;
    }

    private boolean genVS(
            Solution solution,
            String arch,
            String vsVersion,
            String toolset) {

        final String METHOD_NAME = "genVS";
        boolean returnedValue = false;

        StringTemplateGroup vsTemplates = StringTemplateGroup.loadGroup("VS", DefaultTemplateLexer.class, null);

        if (vsTemplates != null) {
            StringTemplate tsolution = vsTemplates.getInstanceOf("solution");
            StringTemplate tproject = vsTemplates.getInstanceOf("project");
            StringTemplate tprojectFiles = vsTemplates.getInstanceOf("projectFiles");
            StringTemplate tprojectPubSub = vsTemplates.getInstanceOf("projectPubSub");
            StringTemplate tprojectFilesPubSub = vsTemplates.getInstanceOf("projectFilesPubSub");
            StringTemplate tprojectJNI = null;
            StringTemplate tprojectFilesJNI = null;
            if (m_languageOption == LANGUAGE.JAVA || m_languageOption == LANGUAGE.CJ) {
                tprojectJNI = vsTemplates.getInstanceOf("projectJNI");
                tprojectFilesJNI = vsTemplates.getInstanceOf("projectFilesJNI");
            }

            returnedValue = true;

            for (int count = 0; returnedValue && (count < solution.getProjects().size()); ++count) {
                Project project = (Project) solution.getProjects().get(count);

                tproject.setAttribute("solution", solution);
                tproject.setAttribute("project", project);
                tproject.setAttribute("example", m_exampleOption);
                tproject.setAttribute("vsVersion", vsVersion);
                tproject.setAttribute("toolset", toolset);

                tprojectFiles.setAttribute("project", project);
                tprojectFiles.setAttribute("vsVersion", vsVersion);

                tprojectPubSub.setAttribute("solution", solution);
                tprojectPubSub.setAttribute("project", project);
                tprojectPubSub.setAttribute("example", m_exampleOption);
                tprojectPubSub.setAttribute("vsVersion", vsVersion);
                tprojectPubSub.setAttribute("toolset", toolset);

                tprojectFilesPubSub.setAttribute("project", project);
                tprojectFilesPubSub.setAttribute("vsVersion", vsVersion);

                if (m_languageOption == LANGUAGE.JAVA || m_languageOption == LANGUAGE.CJ) {
                    tprojectJNI.setAttribute("solution", solution);
                    tprojectJNI.setAttribute("project", project);
                    tprojectJNI.setAttribute("example", m_exampleOption);
                    tprojectJNI.setAttribute("vsVersion", vsVersion);
                    tprojectJNI.setAttribute("toolset", toolset);

                    tprojectFilesJNI.setAttribute("project", project);
                    tprojectFilesJNI.setAttribute("vsVersion", vsVersion);
                }

                for (int index = 0; index < m_vsconfigurations.length; index++) {
                    tproject.setAttribute("configurations", m_vsconfigurations[index]);
                    tprojectPubSub.setAttribute("configurations", m_vsconfigurations[index]);
                    if (m_languageOption == LANGUAGE.JAVA || m_languageOption == LANGUAGE.CJ) {
                        tprojectJNI.setAttribute("configurations", m_vsconfigurations[index]);
                    }
                }

                if (returnedValue = Utils.writeFile(
                        m_outputDir + project.getName() + "Types-" + m_exampleOption + ".vcxproj",
                        tproject, m_replace)) {
                    if (returnedValue = Utils.writeFile(m_outputDir + project.getName() + "Types-" + m_exampleOption +
                            ".vcxproj.filters", tprojectFiles, m_replace)) {
                        if (project.getHasStruct()) {
                            if (returnedValue = Utils
                                    .writeFile(m_outputDir + project.getName() + "PublisherSubscriber-" +
                                            m_exampleOption + ".vcxproj", tprojectPubSub, m_replace)) {
                                returnedValue = Utils.writeFile(
                                        m_outputDir + project.getName() + "PublisherSubscriber-" + m_exampleOption
                                                + ".vcxproj.filters",
                                        tprojectFilesPubSub,
                                        m_replace);
                            }
                        }
                    }
                }

                if (returnedValue && m_languageOption == LANGUAGE.JAVA || m_languageOption == LANGUAGE.CJ) {
                    if (returnedValue = Utils
                            .writeFile(m_outputDir + project.getName() + "PubSubJNI-" + m_exampleOption +
                                    ".vcxproj", tprojectJNI, m_replace)) {
                        returnedValue = Utils.writeFile(
                                m_outputDir + project.getName() + "PubSubJNI-" + m_exampleOption + ".vcxproj.filters",
                                tprojectFilesJNI,
                                m_replace);
                    }
                }

                tproject.reset();
                tprojectFiles.reset();
                tprojectPubSub.reset();
                tprojectFilesPubSub.reset();
                if (m_languageOption == LANGUAGE.JAVA || m_languageOption == LANGUAGE.CJ) {
                    tprojectJNI.reset();
                    tprojectFilesJNI.reset();
                }

            }

            if (returnedValue) {
                tsolution.setAttribute("solution", solution);
                tsolution.setAttribute("example", m_exampleOption);

                // Project configurations
                for (int index = 0; index < m_vsconfigurations.length; index++) {
                    tsolution.setAttribute("configurations", m_vsconfigurations[index]);
                }

                if (m_languageOption == LANGUAGE.JAVA || m_languageOption == LANGUAGE.CJ) {
                    tsolution.setAttribute("generateJava", true);
                }

                String vsVersion_sol = "2019";
                tsolution.setAttribute("vsVersion", vsVersion_sol);

                returnedValue = Utils.writeFile(m_outputDir + "solution-" + m_exampleOption + ".sln", tsolution,
                        m_replace);
            }

        } else {
            System.out.println("ERROR<" + METHOD_NAME + ">: Cannot load the template group VS");
        }

        return returnedValue;
    }

    private boolean genMakefile(
            Solution solution,
            String arch) {

        boolean returnedValue = false;
        StringTemplate makecxx = null;

        StringTemplateGroup makeTemplates = StringTemplateGroup.loadGroup("makefile", DefaultTemplateLexer.class, null);

        if (makeTemplates != null) {
            makecxx = makeTemplates.getInstanceOf("makecxx");

            makecxx.setAttribute("solution", solution);
            makecxx.setAttribute("example", m_exampleOption);
            makecxx.setAttribute("arch", arch);

            returnedValue = Utils.writeFile(m_outputDir + "makefile_" + m_exampleOption, makecxx, m_replace);

        }

        return returnedValue;
    }

    private boolean genCMakeLists(
            Solution solution) {
        boolean returnedValue = false;
        StringTemplate cmake = null;
        StringTemplateGroup cmakeTemplates;

        if (m_languageOption != LANGUAGE.CJ) {
            cmakeTemplates = StringTemplateGroup.loadGroup("CMakeLists", DefaultTemplateLexer.class,
                    null);
        } else {
            solution.setFileName(m_onlyFileName);
            cmakeTemplates = StringTemplateGroup.loadGroup("CMakeListsForCJ", DefaultTemplateLexer.class,
                    null);
        }

        solution.setUseVbsFrameworkFlag(m_use_vbs_framework);
        solution.setGenSharedLib(m_gen_shared_lib);
        solution.setZeroCopyFlag(m_use_zero_copy);
        solution.setTestExampleFlag(m_test_example);
        solution.setStaticXmlFlag(m_use_static_xml);
        solution.setRmPrintHelper(m_not_use_print_help);
        solution.setHasMutable(m_hasMutable);
        solution.setGenBoth(m_gen_both_for_rti);
        if (cmakeTemplates != null) {
            cmake = cmakeTemplates.getInstanceOf("cmakelists");

            cmake.setAttribute("solution", solution);
            cmake.setAttribute("pure_flag", m_pure_structure);
            cmake.setAttribute("test", m_test);
            returnedValue = Utils.writeFile(m_outputDir + "CMakeLists.txt", cmake, m_replace);
        }
        return returnedValue;
    }

    private boolean genSwigCMake(
            Solution solution) {

        boolean returnedValue = false;
        StringTemplate swig = null;

        StringTemplateGroup swigTemplates = StringTemplateGroup.loadGroup("SwigCMake", DefaultTemplateLexer.class,
                null);
        if (swigTemplates != null) {
            swig = swigTemplates.getInstanceOf("swig_cmake");

            swig.setAttribute("solution", solution);

            returnedValue = Utils.writeFile(m_outputDir + "CMakeLists.txt", swig, m_replace);

        }
        return returnedValue;
    }

    String callPreprocessor(
            String idlFilename) {
        final String METHOD_NAME = "callPreprocessor";

        // Set line command.
        ArrayList<String> lineCommand = new ArrayList<String>();
        String[] lineCommandArray = null;
        String outputfile = Util.getIDLFileOnly(idlFilename) + ".cc";
        int exitVal = -1;
        OutputStream of = null;

        // Use temp directory.
        if (m_tempDir != null) {
            outputfile = m_tempDir + outputfile;
        }

        if (m_os.contains("Windows")) {
            try {
                of = new FileOutputStream(outputfile);
            } catch (FileNotFoundException ex) {
                System.out.println(ColorMessage.error(METHOD_NAME) + "Cannot open file " + outputfile);
                return null;
            }
        }

        // Set the preprocessor path
        String ppPath = m_ppPath;

        if (ppPath == null) {
            if (m_os.contains("Windows")) {
                ppPath = "cl.exe";
            } else if (m_os.contains("Linux") || m_os.contains("Mac")) {
                ppPath = "cpp";
            }
        }

        // Add command
        lineCommand.add(ppPath);

        // Add the include paths given as parameters.
        for (int i = 0; i < m_includePaths.size(); ++i) {
            if (m_os.contains("Windows")) {
                lineCommand.add(((String) m_includePaths.get(i)).replaceFirst("^-I", "/I"));
            } else if (m_os.contains("Linux") || m_os.contains("Mac")) {
                lineCommand.add(m_includePaths.get(i));
            }
        }

        if (ppPath.equals("cl.exe") && m_os.contains("Windows")) {
            lineCommand.add("/E");
            lineCommand.add("/C");
        }

        // Add input file.
        lineCommand.add(idlFilename);

        if (m_os.contains("Linux") || m_os.contains("Mac")) {
            lineCommand.add(outputfile);
        }

        lineCommandArray = new String[lineCommand.size()];
        lineCommandArray = (String[]) lineCommand.toArray(lineCommandArray);

        try {
            Process preprocessor = Runtime.getRuntime().exec(lineCommandArray);
            ProcessOutput errorOutput = new ProcessOutput(preprocessor.getErrorStream(), "ERROR", false, null, true);
            ProcessOutput normalOutput = new ProcessOutput(preprocessor.getInputStream(), "OUTPUT", false, of, true);
            errorOutput.start();
            normalOutput.start();
            exitVal = preprocessor.waitFor();
            errorOutput.join();
            normalOutput.join();
        } catch (Exception e) {
            System.out.println(ColorMessage.error(
                    METHOD_NAME) + "Cannot execute the preprocessor. Reason: " + e.getMessage());
            return null;
        }

        if (of != null) {
            try {
                of.close();
            } catch (IOException e) {
                System.out.println(ColorMessage.error(METHOD_NAME) + "Cannot close file " + outputfile);
            }

        }

        if (exitVal != 0) {
            System.out.println(ColorMessage.error(METHOD_NAME) + "Preprocessor return an error " + exitVal);
            return null;
        }

        return outputfile;
    }

    boolean callJavah(
            String idlFilename) {
        final String METHOD_NAME = "calljavah";
        // Set line command.
        ArrayList<String> lineCommand = new ArrayList<String>();
        String[] lineCommandArray = null;
        String fileDir = Util.getIDLFileDirectoryOnly(idlFilename);
        String javafile = (m_outputDir != null ? m_outputDir : "") +
                (!m_package.isEmpty() ? m_package.replace('.', File.separatorChar) + File.separator : "") +
                Util.getIDLFileNameOnly(idlFilename) + "PubSub.java";
        String headerfile = m_outputDir + Util.getIDLFileNameOnly(idlFilename) + "PubSubJNI"+(m_gen_hpp_file ? ".hpp" : ".h");
        int exitVal = -1;
        String javac = null;
        String javah = null;

        // First call javac
        if (m_os.contains("Windows")) {
            javac = "javac.exe";
        } else if (m_os.contains("Linux") || m_os.contains("Mac")) {
            javac = "javac";
        }

        // Add command
        lineCommand.add(javac);
        if (m_tempDir != null) {
            lineCommand.add("-d");
            lineCommand.add(m_tempDir);
        }

        if (fileDir != null && !fileDir.isEmpty()) {
            lineCommand.add("-sourcepath");
            lineCommand.add(m_outputDir);
        }

        lineCommand.add(javafile);

        lineCommandArray = new String[lineCommand.size()];
        lineCommandArray = (String[]) lineCommand.toArray(lineCommandArray);

        try {
            Process preprocessor = Runtime.getRuntime().exec(lineCommandArray);
            ProcessOutput errorOutput = new ProcessOutput(preprocessor.getErrorStream(), "ERROR", false, null, true);
            ProcessOutput normalOutput = new ProcessOutput(preprocessor.getInputStream(), "OUTPUT", false, null, true);
            errorOutput.start();
            normalOutput.start();
            exitVal = preprocessor.waitFor();
            errorOutput.join();
            normalOutput.join();
        } catch (Exception ex) {
            System.out.println(ColorMessage.error(
                    METHOD_NAME) + "Cannot execute the javac application. Reason: " + ex.getMessage());
            return false;
        }

        if (exitVal != 0) {
            System.out.println(ColorMessage.error(METHOD_NAME) + "javac application return an error " + exitVal);
            return false;
        }

        lineCommand = new ArrayList<String>();

        if (m_os.contains("Windows")) {
            javah = "javah.exe";
        } else if (m_os.contains("Linux") || m_os.contains("Mac")) {
            javah = "javah";
        }

        // Add command
        lineCommand.add(javah);
        lineCommand.add("-jni");
        if (m_tempDir != null) {
            lineCommand.add("-cp");
            lineCommand.add(m_tempDir);
        }
        lineCommand.add("-o");
        lineCommand.add(headerfile);
        lineCommand.add((!m_package.isEmpty() ? m_package + "." : "") +
                Util.getIDLFileNameOnly(idlFilename) + "PubSub");

        lineCommandArray = new String[lineCommand.size()];
        lineCommandArray = (String[]) lineCommand.toArray(lineCommandArray);

        try {
            Process preprocessor = Runtime.getRuntime().exec(lineCommandArray);
            ProcessOutput errorOutput = new ProcessOutput(preprocessor.getErrorStream(), "ERROR", false, null, true);
            ProcessOutput normalOutput = new ProcessOutput(preprocessor.getInputStream(), "OUTPUT", false, null, true);
            errorOutput.start();
            normalOutput.start();
            exitVal = preprocessor.waitFor();
            errorOutput.join();
            normalOutput.join();
        } catch (Exception ex) {
            System.out.println(ColorMessage.error(
                    METHOD_NAME) + "Cannot execute the javah application. Reason: " + ex.getMessage());
            return false;
        }

        if (exitVal != 0) {
            System.out.println(ColorMessage.error(METHOD_NAME) + "javah application return an error " + exitVal);
            return false;
        }

        return true;
    }

    public enum IDLTypes {
        DDSGen,
        RPCGen,
        errorCode
    };

    /*
     * -----------------------------------------------------------------------------
     * -----------
     *
     * Main entry point
     */

    public static void main(
            String[] args) {
        String[] argsTmp = args;
        String arg = null;
        String m_type = null;
        String m_language = null;
        int count = 0;
        IDLTypes genType = IDLTypes.errorCode;
        String homeDirectory = System.getProperty("user.home");

        ColorMessage.load();

        if (loadPlatforms()) {
            while (count < argsTmp.length) {
                arg = argsTmp[count++];

                if (arg.equals("-language")) {
                    m_language = argsTmp[count++];
                    if (m_language.equals("C")) {
                        try {
                            int exitVal = -1;
                            String rtiGen = "./rtiddsgen/scripts/rtiddsgen";
                            int i = 0;
                            while (i < args.length) {
                                rtiGen = rtiGen.concat(" ");
                                rtiGen = rtiGen.concat(args[i++]);
                            }
                            Process preprocessor = Runtime.getRuntime().exec(new String[] { "/bin/sh", "-c", rtiGen });
                            ProcessOutput errorOutput = new ProcessOutput(preprocessor.getErrorStream(), "ERROR", false,
                                    null, true);
                            ProcessOutput normalOutput = new ProcessOutput(preprocessor.getInputStream(), "OUTPUT",
                                    false, null, true);
                            errorOutput.start();
                            normalOutput.start();
                            exitVal = preprocessor.waitFor();
                            errorOutput.join();
                            normalOutput.join();
                            System.exit(-1);
                        }

                        catch (Exception ex) {
                            System.out.println(ColorMessage.error(
                                    "Execution error") + "Cannot execute the command line correctly. Reason: "
                                    + ex.getMessage());
                            printHelp();
                            System.exit(-1);
                        }

                    }
                }

            }

            count = 0;
            while (count < argsTmp.length) {
                arg = argsTmp[count++];

                if (arg.equals("-type")) {
                    m_type = argsTmp[count++];
                    if (m_type.equals("DDS")) {
                        genType = IDLTypes.DDSGen;
                    } else if (m_type.equals("RPC")) {
                        genType = IDLTypes.RPCGen;
                    } else {
                        genType = IDLTypes.errorCode;
                        System.out.println("ERROR<BadArgument>:Please enter the correct type");
                    }
                    break;
                }
            }

            if (genType == IDLTypes.DDSGen) {
                try {
                    vbsddsgen main = new vbsddsgen(args);
                    if (main.execute()) {
                        System.exit(0);
                    }
                } catch (BadArgumentException e) {
                    System.out.println(ColorMessage.error("BadArgumentException") + e.getMessage());
                    printHelp();
                }
            } else if (genType == IDLTypes.RPCGen) {
                try {
                    vbsrpcgen main = new vbsrpcgen(args);
                    if (main.execute()) {
                        System.exit(0);
                    }
                } catch (Exception ex) {
                    System.out.println(ColorMessage.error("BadArgumentException") + ex.getMessage());
                    printHelp();
                }
            } else {
                printHelp();

            }

        }

        System.exit(-1);
    }

    public static ArrayList<String> getXmlString(
            String[] args) {
        String[] argsTmp = args;
        String arg = null;
        String m_type = null;
        String m_language = null;
        int count = 0;
        IDLTypes genType = IDLTypes.errorCode;
        String homeDirectory = System.getProperty("user.home");

        ColorMessage.load();

        if (loadPlatforms()) {

            count = 0;
            while (count < argsTmp.length) {
                arg = argsTmp[count++];

                if (arg.equals("-type")) {
                    m_type = argsTmp[count++];
                    if (m_type.equals("DDS")) {
                        genType = IDLTypes.DDSGen;
                    } else if (m_type.equals("RPC")) {
                        genType = IDLTypes.RPCGen;
                    } else {
                        genType = IDLTypes.errorCode;
                        System.out.println("ERROR<BadArgument>:Please enter the correct type");
                    }
                    break;
                }
            }

            if (genType == IDLTypes.DDSGen) {
                try {
                    vbsddsgen main = new vbsddsgen(args);
                    if (main.execute()) {
                        return main.getXmlArray();
                    }
                } catch (BadArgumentException e) {
                    System.out.println(ColorMessage.error("BadArgumentException") + e.getMessage());
                    printHelp();
                }
            } else if (genType == IDLTypes.RPCGen) {
                try {
                    vbsrpcgen main = new vbsrpcgen(args);
                    if (main.execute()) {
                        return null;
                    }
                } catch (Exception ex) {
                    System.out.println(ColorMessage.error("BadArgumentException") + ex.getMessage());
                    printHelp();
                }
            } else {
                printHelp();

            }

        }
        return null;
    }

    private boolean genCargoBuild(Solution solution, String arch) {
        boolean returnedValue = false;
        StringTemplate CargoBuildxx = null;

        StringTemplateGroup rustTemplates = StringTemplateGroup.loadGroup("Rustbuild", DefaultTemplateLexer.class, null);

        solution.setGenSharedLib(m_gen_shared_lib);
        solution.setRmPrintHelper(m_not_use_print_help);
        solution.setTestExampleFlag(m_test_example);
        if (rustTemplates != null) {
            CargoBuildxx = rustTemplates.getInstanceOf("Cargobuild");

            CargoBuildxx.setAttribute("solution", solution);
            CargoBuildxx.setAttribute("dds", "dds");
            returnedValue = Utils.writeFile(m_rustoutputDir + "build.rs", CargoBuildxx, m_replace);
        }
        return returnedValue;
    }

}

class ProcessOutput extends Thread {
    InputStream is = null;
    OutputStream of = null;
    String type;
    boolean m_check_failures;
    boolean m_found_error = false;
    final String clLine = "#line";
    boolean m_printLine = false;

    ProcessOutput(
            InputStream is,
            String type,
            boolean check_failures,
            OutputStream of,
            boolean printLine) {
        this.is = is;
        this.type = type;
        m_check_failures = check_failures;
        this.of = of;
        m_printLine = printLine;
    }

    public void run() {
        try {
            InputStreamReader isr = new InputStreamReader(is);
            BufferedReader br = new BufferedReader(isr);
            String line = null;
            while ((line = br.readLine()) != null) {
                if (of == null) {
                    if (m_printLine) {
                        System.out.println(line);
                    }
                } else {
                    // Sustituir los "\\" que pone cl.exe por "\"
                    if (line.startsWith(clLine)) {
                        line = "#" + line.substring(clLine.length());
                        int count = 0;
                        while ((count = line.indexOf("\\\\")) != -1) {
                            line = line.substring(0, count) + "\\" + line.substring(count + 2);
                        }
                    }

                    of.write(line.getBytes());
                    of.write('\n');
                }

                if (m_check_failures) {
                    if (line.startsWith("Done (failures)")) {
                        m_found_error = true;
                    }
                }
            }
        } catch (IOException ioe) {
            ioe.printStackTrace();
        }
    }

    boolean getFoundError() {
        return m_found_error;
    }

}
