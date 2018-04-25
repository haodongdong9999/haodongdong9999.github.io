set nocompatible              " be iMproved, required
filetype off                  " required

" set the runtime path to include Vundle and initialize
set rtp+=~/.vim/bundle/Vundle.vim
call vundle#begin()
" alternatively, pass a path where Vundle should install plugins
"call vundle#begin('~/some/path/here')

" let Vundle manage Vundle, required
Plugin 'gmarik/Vundle.vim'
Plugin 'taglist.vim'
Plugin 'ctrlp.vim'
Plugin 'simplyzhao/cscope_maps.vim'
Plugin 'autotags'
Plugin 'DirDiff.vim'
Plugin 'omnicppcomplete'
Plugin 'bufexplorer.zip'
Plugin 'code_complete'
Plugin 'c.vim'
Plugin 'winmanager'
Plugin 'trailing-whitespace'
Plugin 'align'
Plugin 'bling/vim-airline'
Plugin 'altercation/vim-colors-solarized'
Plugin 'dagwieers/asciidoc-vim'
Plugin 'airblade/vim-gitgutter'
Plugin 'jiangmiao/auto-pairs'
Plugin 'vim-airline/vim-airline-themes'
" All of your Plugins must be added before the following line
call vundle#end()            " required
filetype plugin indent on    " required
" To ignore plugin indent changes, instead use:
"filetype plugin on
"
" Brief help
" :PluginList       - lists configured plugins
" :PluginInstall    - installs plugins; append `!` to update or just :PluginUpdate
" :PluginSearch foo - searches for foo; append `!` to refresh local cache
" :PluginClean      - confirms removal of unused plugins; append `!` to auto-approve removal
"
" see :h vundle for more details or wiki for FAQ
" Put your non-Plugin stuff after this line



" An example for a vimrc file.
"
" Maintainer:	Bram Moolenaar <Bram@vim.org>
" Last change:	2011 Apr 15
"
" To use it, copy it to
"     for Unix and OS/2:  ~/.vimrc
"	      for Amiga:  s:.vimrc
"  for MS-DOS and Win32:  $VIM\_vimrc
"	    for OpenVMS:  sys$login:.vimrc

" When started as "evim", evim.vim will already have done these settings.
if v:progname =~? "evim"
  finish
endif
let &termencoding=&encoding
set fileencodings=utf-8,gbk
" Use Vim settings, rather than Vi settings (much better!).
" This must be first, because it changes other options as a side effect.
set nocompatible
set wildmenu

" allow backspacing over everything in insert mode
set backspace=indent,eol,start

if has("vms")
  set nobackup		" do not keep a backup file, use versions instead
else
  set nobackup		" keep a backup file
endif
set history=50		" keep 50 lines of command line history
set ruler		" show the cursor position all the time
set showcmd		" display incomplete commands
set incsearch		" do incremental searching

" For Win32 GUI: remove 't' flag from 'guioptions': no tearoff menu entries
" let &guioptions = substitute(&guioptions, "t", "", "g")

" Don't use Ex mode, use Q for formatting
map Q gq

" CTRL-U in insert mode deletes a lot.  Use CTRL-G u to first break undo,
" so that you can undo CTRL-U after inserting a line break.
inoremap <C-U> <C-G>u<C-U>

" In many terminal emulators the mouse works just fine, thus enable it.
"if $TERM !=”linux”
"  set mouse=a
"endif

" Switch syntax highlighting on, when the terminal has colors
" Also switch on highlighting the last used search pattern.
if &t_Co > 2 || has("gui_running")
  syntax on
  set hlsearch
endif

" Only do this part when compiled with support for autocommands.
if has("autocmd")

  " Enable file type detection.
  " Use the default filetype settings, so that mail gets 'tw' set to 72,
  " 'cindent' is on in C files, etc.
  " Also load indent files, to automatically do language-dependent indenting.
  filetype plugin indent on

  " Put these in an autocmd group, so that we can delete them easily.
  augroup vimrcEx
  au!

  " For all text files set 'textwidth' to 78 characters.
  autocmd FileType text setlocal textwidth=78

  " When editing a file, always jump to the last known cursor position.
  " Don't do it when the position is invalid or when inside an event handler
  " (happens when dropping a file on gvim).
  " Also don't do it when the mark is in the first line, that is the default
  " position when opening a file.
  autocmd BufReadPost *
    \ if line("'\"") > 1 && line("'\"") <= line("$") |
    \   exe "normal! g`\"" |
    \ endif

  augroup END

else

  set autoindent		" always set autoindenting on

endif " has("autocmd")

" Convenient command to see the difference between the current buffer and the
" file it was loaded from, thus the changes you made.
" Only define it when not defined already.
if !exists(":DiffOrig")
  command DiffOrig vert new | set bt=nofile | r ++edit # | 0d_ | diffthis
		  \ | wincmd p | diffthis
endif

" For Tlist, by fangz
let Tlist_Use_Right_Window = 1
let Tlist_WinWidth = 35
let Tlist_Exit_OnlyWindow = 1
let Tlist_Show_One_File = 1
let g:ctrlp_by_filename = 1
let g:ctrlp_max_height = 15
let NERDTreeWinSize=20

set path+=$PWD/**
" add c smart edit,by fangz
set smartindent
" add edit keep tab = 4 space, by fangz
set cindent
set tabstop=4
set shiftwidth=4
set expandtab

"nmap <leader>t :cstag<space>
"nmap <leader>c :c<CR>
nmap ,- :Explore<CR>
nmap ,b :cprevious<CR>
nmap ,m :Tlist<CR>
nmap ,n :cnext<CR>
nmap ,f :find<space>
nmap ,q :q<CR>
nmap <space><space> :Fix<CR>
nmap <leader>w :WMToggle<CR>
nmap <leader>o :only<CR>
nmap <leader>f :find<CR>

nmap <space>[ :tabprevious<CR>
nmap <space>] :tabNext<CR>
nmap <space>t :tabedit<space>
nmap <space>= vip=<CR>
nmap <space>w ciw
nmap <C-j> <C-W>j
nmap <C-h> <C-W>h
nmap <C-k> <C-W>k
nmap <C-l> <C-W>l

let g:ctrlp_working_path_mode = 'ra'

set clipboard=unnamedplus

"syntax enable
"set background=dark
"colorscheme solarized
"let g:solarized_termcolors=256
colorscheme ron

set completeopt=menu

"gvim conf
set guifont=文泉驿等宽正黑\ Medium\ 12
if has("gui_running")
    set background=dark
    let g:solarized_termcolors=256
    colorscheme solarized
    set guicursor+=n-v-c:blinkon0
else
endif
set laststatus=2
set relativenumber

let g:airline_theme='solarized'
let g:airline_solarized_bg='light'

autocmd BufRead,BufNewFile *.adoc set filetype=asciidoc
