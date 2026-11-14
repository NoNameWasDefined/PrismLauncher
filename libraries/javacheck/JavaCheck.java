// File overview: Tiny program run by the launcher's JavaChecker with each candidate Java binary: prints system
// properties (version, arch, vendor) so the launcher can identify that Java.

public final class JavaCheck {
    private static final String[] CHECKED_PROPERTIES = new String[] {"os.arch", "java.version", "java.vendor"};

    public static void main(String[] args) {
        int returnCode = 0;

        for (String key : CHECKED_PROPERTIES) {
            String property = System.getProperty(key);

            if (property != null) {
                System.out.println(key + "=" + property);
            } else {
                returnCode = 1;
            }
        }

        System.exit(returnCode);
    }
}
