{
  "targets": [
    {
      "target_name": "addon",
      'sources' : [  "<!@(node -p \"require('fs').readdirSync('../code/SoftRenderer').map(f=>'../code/SoftRenderer/'+f).join(' ')\")" ],
      "include_dirs" : ["<!(node -e \"require('nan')\")","../code/SoftRenderer/"],
      'conditions': [
        [
          'OS=="win"', {   
              "libraries": [ "winmm.lib" ]
            },
        'OS=="mac"', {}
        ]
      ]
    }
  ]
}
