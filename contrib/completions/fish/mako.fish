function complete_outputs
    if string length -q "$SWAYSOCK"; and command -sq jq
        swaymsg -t get_outputs | jq -r '.[] | select(.active) | "\(.name)\t\(.make) \(.model)"'
    else
        return 1
    end
end

complete -c mako -s h -l help -d 'Show help and exit'
complete -c mako -s c -l config -d 'Path to config file' -r
complete -c mako -l font -d 'Font family and size' -x
complete -c mako -l background-color -d 'Background color in #RRGGBB[AA]' -x
complete -c mako -l text-color -d 'Text color in #RRGGBB[AA]' -x
complete -c mako -l width -d 'Notification width in px' -x
complete -c mako -l height -d 'Max notification height in px' -x
complete -c mako -l outer-margin -d 'Outer margin values in px, comma separated' -x
complete -c mako -l margin -d 'Margin values in px, comma separated' -x
complete -c mako -l padding -d 'Padding values in px, comma separated' -x
complete -c mako -l border-size -d 'Border size in px' -x
complete -c mako -l border-color -d 'Border color in #RRGGBB[AA]' -x
complete -c mako -l border-radius -d 'Border radius values in px,comma separated' -x
complete -c mako -l progress-color -d 'Progress color indicator' -x
complete -c mako -l icons -d 'Show icons or not' -xa "1 0"
complete -c mako -l icon-location -d 'Position of the icon relative to the text' -xa 'left right top bottom'
complete -c mako -l icon-path -d 'Icon search path, colon delimited' -r
complete -c mako -l max-icon-size -d 'Max icon size in px' -x
complete -c mako -l icon-border-radius -d 'Icon border radius value in px' -x
complete -c mako -l markup -d 'Enable markup or not' -xa "1 0"
complete -c mako -l actions -d 'Enable actions or not' -xa "1 0"
complete -c mako -l format -d 'Format string' -x
complete -c mako -l max-visible -d 'Max visible notifications' -x
complete -c mako -l sort -d 'Set notification sorting method' -x
complete -c mako -l group-by -d 'Group notifications sharing these criteria fields' -x
complete -c mako -l default-timeout -d 'Notification timeout in ms' -x
complete -c mako -l ignore-timeout -d 'Enable notification timeout or not' -xa "1 0"
complete -c mako -l ignore-replace -d 'Ignore requests to replace a notification' -xa "1 0"
complete -c mako -l ignore-close -d 'Ignore requests to close a notification' -xa "1 0"
complete -c mako -l output -d 'Show notifications on this output' -xa '(complete_outputs)'
complete -c mako -l layer -d 'Show notifications on this layer' -x
complete -c mako -l anchor -d 'Position on output to put notifications' -x
complete -c mako -l on-button-left -d 'Action on left pointer button' -xa "none dismiss dismiss-all dismiss-group invoke-default-action invoke-action exec"
complete -c mako -l on-button-middle -d 'Action on middle pointer button' -xa "none dismiss dismiss-all dismiss-group invoke-default-action invoke-action exec"
complete -c mako -l on-button-right -d 'Action on right pointer button' -xa "none dismiss dismiss-all dismiss-group invoke-default-action invoke-action exec"
complete -c mako -l on-touch -d 'Action on touch' -xa "none dismiss dismiss-all dismiss-group invoke-default-action invoke-action exec"
