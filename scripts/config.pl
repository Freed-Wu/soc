#!/usr/bin/env -S perl -pi
# scripts/config.pl /the/path/of/source/config project-spec/configs/config
unless ( $#ARGV < 0 ) {
    unless ( $_ =~ '^#' ) {
        /^([^=]*)(?{ $k = $^N })=(.*)(?{ $v = $^N })/;
        $config{$k} = $v;
    }
    next;
}
while ( my ( $k, $v ) = each %config ) {
    s/$k=.*/$k=$v/;
    s/# $k is not set/$k=$v/;
}
