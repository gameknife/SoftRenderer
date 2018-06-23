{
  "targets": [{
    "target_name": "addon",
    "sources": [
      "./*.cpp",
      "../code/SoftRenderer/*.cpp"
    ],
    "include_dirs" : ["<!(node -e \"require('nan')\")","../code/SoftRenderer/"],
    "libraries": [ "winmm.lib" ]
  }]
}
