{
  "targets": [
    {
      "target_name": "addon",
      'sources' : [
        "<!@(node -p \"require('fs').readdirSync('../code/SoftRenderer').map(f=>'../code/SoftRenderer/'+f).join(' ')\")"
      ],
      "include_dirs": [
        "<!(node -e \"require('nan')\")",
        "../code/SoftRenderer/"
      ],
      
      'conditions': [
        [
          'OS=="win"',
          {
            "libraries": [
              "winmm.lib"
            ],
              'msvs_settings': {
                'VCCLCompilerTool': {
                "ExceptionHandling": 1,
                  'FloatingPointModel': 2,
                  'EnableEnhancedInstructionSet' : 5,
                  'StructMemberAlignment' : 5,
                  'Optimization' : 2,
              },
                'VCLinkerTool': {
                  'LinkTimeCodeGeneration': 1,
                  'OptimizeReferences': 2,
                  'EnableCOMDATFolding': 2,
                  'LinkIncremental': 1,
              }
            }
          },
        'OS=="mac"',
          {
          'xcode_settings': {
        'OTHER_CFLAGS': [
              '-mavx',
              '-mfma',
              '-Ofast'
              ],
            },
          },
        'OS=="linux"',
          {
          'cflags': ['-mavx','-mfma','-Ofast','-ffast-math', '-O2'
            ],
          }
        ]
      ]
    }
  ]
}