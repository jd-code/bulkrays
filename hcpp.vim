let b:current_syntax = ''
unlet b:current_syntax
runtime! syntax/cpp.vim

let b:current_syntax = ''
unlet b:current_syntax
syntax include @CpP syntax/cpp.vim

let b:current_syntax = ''
unlet b:current_syntax
syntax include @HtMl syntax/html.vim
syntax region htmlCode matchgroup=Snip start="//html{" end="//}html" containedin=@CpP contains=@HtMl
syntax region cppCode matchgroup=Snip start="//}html" end="//html{" containedin=@HtMl contains=@CpP

syntax region htmlCode1 matchgroup=Snip start="{{" end="}}" containedin=@CpP contains=@HtMl
syntax region cppCode1 matchgroup=Snip start="}}" end="{{" containedin=@HtMl contains=@CpP
syntax region cppCode2 matchgroup=Snip start="}}" end="{{" containedin=htmlString contains=@CpP

syntax region cppCode3 matchgroup=Snip start="}}" end="{{" containedin=htmlCss contains=@CpP
syntax region cppCode4 matchgroup=Snip start="}}" end="{{" containedin=cssStringQ contains=@CpP
syntax region cppCode5 matchgroup=Snip start="}}" end="{{" containedin=cssFontDescriptorFunction contains=@CpP

"syntax region htmlCode matchgroup=Snip start="{<" end=">}" containedin=@CpP contains=@HtMl
"syntax region htmlCode matchgroup=Snip start=">}" end="{<" containedin=@HtMl contains=@CpP
"syntax region htmlCode matchgroup=Snip start=">}" end="{<" containedin=htmlString contains=@CpP

" hi link Snip SpecialChar
hi Snip cterm=NONE ctermfg=white ctermbg=blue


let b:current_syntax = 'hcpp'
