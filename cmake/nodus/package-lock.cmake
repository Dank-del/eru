# CPM Package Lock
# This file should be committed to version control

# cli11
CPMDeclarePackage(cli11
  NAME cli11
  GIT_TAG v2.4.2
  GITHUB_REPOSITORY CLIUtils/CLI11
)
# cpr
CPMDeclarePackage(cpr
  NAME cpr
  GIT_TAG 1.10.x
  GITHUB_REPOSITORY libcpr/cpr
  OPTIONS
    "CPR_CURL_USE_LIBPSL OFF"
)
# indicators
CPMDeclarePackage(indicators
  NAME indicators
  GIT_TAG v2.3
  GITHUB_REPOSITORY p-ranav/indicators
)
