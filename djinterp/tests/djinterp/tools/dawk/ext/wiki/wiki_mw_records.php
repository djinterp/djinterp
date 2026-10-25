<?php
// wiki_mw_records.php -- what MediaWiki itself makes of pages, for
// wiki_oracle.py to compare the wiki reader's records with.
//
// Each file named is parsed as the page "Oracle test", and printed as one
// JSON line: its sections -- level, anchor, byte offset, title -- and what
// ParserOutput records it links to: pages, templates, files, categories,
// interwiki, language and special pages, each as [namespace, dbkey,
// interwiki prefix], and its external links. The categories MediaWiki puts
// a page in by itself, its tracking categories, are left out.
//   With --titles, it reads JSON lines from its input instead, each
// {"text": ..., "ns": ...}, and prints each title as MediaWiki resolves it,
// [namespace, dbkey, interwiki prefix], or null for one no title can be:
// "ns" is the namespace a name with none is in, 10 for a template's. A line
// {"url": ...} is printed as MediaWiki records an external link to it.
//   With --magic, it prints the names of the parser functions installed,
// as written before their `:`, those whose case is their own apart.
//
//   MW_INSTALL_PATH=<mediawiki> php <mediawiki>/maintenance/run.php \
//       wiki_mw_records.php [--titles | --magic] [FILE...]
//
// The wiki is any MediaWiki 1.43 installed, ParserFunctions not among its
// extensions -- so {{#if:...}} stays text, every branch of it read, as the
// reader reads a page's source -- with these extension tags registered:
//
//   $wgHooks['ParserFirstCallInit'][] = static function ( $parser ) {
//       foreach ( [ 'ref', 'references', 'poem' ] as $tag ) {
//           $parser->setHook( $tag, static fn ( $in, $args, $p, $frame ) =>
//               $p->recursiveTagParse( (string)$in, $frame ) );
//       }
//       foreach ( [ 'syntaxhighlight', 'source', 'math', 'chem', 'score',
//                   'timeline', 'graph', 'templatedata' ] as $tag ) {
//           $parser->setHook( $tag, static fn ( $in ) =>
//               htmlspecialchars( (string)$in ) );
//       }
//   };

use MediaWiki\MediaWikiServices;
use MediaWiki\Parser\Parser;
use MediaWiki\Parser\ParserOutputLinkTypes;
use MediaWiki\Parser\Sanitizer;
use MediaWiki\Title\Title;

require_once getenv( 'MW_INSTALL_PATH' ) . '/maintenance/Maintenance.php';

class WikiReaderRecords extends Maintenance {
    public function __construct() {
        parent::__construct();
        $this->addOption( 'titles', 'resolve titles read from the input' );
        $this->addOption( 'magic', 'print the parser functions installed' );
        $this->addArg( 'files', 'the pages to parse', false, true );
    }

    /** Each link of a type, as [namespace, dbkey, interwiki prefix]. */
    private static function targets( $output, string $type ): array {
        $out = [];
        foreach ( $output->getLinkList( $type ) as $entry ) {
            $link = $entry['link'];
            $out[] = [ $link->getNamespace(), $link->getDBkey(),
                $link->getInterwiki() ];
        }
        return $out;
    }

    /**
     * Each title read, as MediaWiki resolves it; or, with "url", each URL as
     * an external link to it is recorded.
     */
    private function titles(): void {
        while ( ( $line = fgets( STDIN ) ) !== false ) {
            $asked = json_decode( $line, true );
            if ( isset( $asked['url'] ) ) {
                echo json_encode( Parser::normalizeLinkUrl(
                    Sanitizer::cleanUrl( (string)$asked['url'] ) ),
                    JSON_UNESCAPED_UNICODE | JSON_UNESCAPED_SLASHES ) . "\n";
                continue;
            }
            $title = Title::newFromText( (string)$asked['text'],
                (int)$asked['ns'] );
            echo json_encode( ( $title === null ) ? null : [
                $title->getNamespace(), $title->getDBkey(),
                $title->getInterwiki() ],
                JSON_UNESCAPED_UNICODE | JSON_UNESCAPED_SLASHES ) . "\n";
        }
    }

    /** The parser functions' names, as written before their `:`. */
    private function magic(): void {
        $parser = MediaWikiServices::getInstance()->getParserFactory()
            ->create();
        $parser->firstCallInit();
        $synonyms = ( new ReflectionProperty( $parser, 'mFunctionSynonyms' ) )
            ->getValue( $parser );
        echo json_encode( [
            'insensitive' => array_map( 'strval', array_keys( $synonyms[0] ) ),
            'sensitive' => array_map( 'strval', array_keys( $synonyms[1] ) ),
        ], JSON_UNESCAPED_UNICODE | JSON_UNESCAPED_SLASHES ) . "\n";
    }

    /** A page's sections, without their HTML's markup. */
    private static function sections( $output ): array {
        $out = [];
        foreach ( $output->getSections() as $s ) {
            $out[] = [ 'level' => (int)$s['level'], 'anchor' => $s['anchor'],
                'offset' => $s['byteoffset'],
                'line' => html_entity_decode( strip_tags( $s['line'] ),
                    ENT_QUOTES | ENT_HTML5, 'UTF-8' ) ];
        }
        return $out;
    }

    public function execute() {
        $services = MediaWikiServices::getInstance();
        $title = Title::newFromText( 'Oracle test' );
        $tracking = [];

        if ( $this->hasOption( 'titles' ) ) {
            $this->titles();
            return;
        }
        if ( $this->hasOption( 'magic' ) ) {
            $this->magic();
            return;
        }
        // the categories MediaWiki puts a page in by itself
        foreach ( $services->getTrackingCategories()->getTrackingCategories()
                as $info ) {
            foreach ( $info['cats'] as $cat ) {
                $tracking[$cat->getDBkey()] = true;
            }
        }
        foreach ( $this->getArgs() as $file ) {
            $parser = $services->getParserFactory()->create();
            $text = file_get_contents( $file );
            try {
                $output = $parser->parse( $text, $title,
                    ParserOptions::newFromAnon(), true, true );
            } catch ( Throwable $e ) {
                echo json_encode( [ 'file' => $file,
                    'error' => $e->getMessage() ],
                    JSON_UNESCAPED_UNICODE | JSON_UNESCAPED_SLASHES ) . "\n";
                continue;
            }
            echo json_encode( [
                'file' => $file,
                'sections' => self::sections( $output ),
                'links' => self::targets( $output,
                    ParserOutputLinkTypes::LOCAL ),
                'templates' => self::targets( $output,
                    ParserOutputLinkTypes::TEMPLATE ),
                'images' => self::targets( $output,
                    ParserOutputLinkTypes::MEDIA ),
                'categories' => array_values( array_filter(
                    self::targets( $output, ParserOutputLinkTypes::CATEGORY ),
                    static fn ( $c ) => !isset( $tracking[$c[1]] ) ) ),
                'interwiki' => self::targets( $output,
                    ParserOutputLinkTypes::INTERWIKI ),
                'language' => self::targets( $output,
                    ParserOutputLinkTypes::LANGUAGE ),
                'special' => self::targets( $output,
                    ParserOutputLinkTypes::SPECIAL ),
                'external' => array_map( 'strval',
                    array_keys( $output->getExternalLinks() ) ),
            ], JSON_UNESCAPED_UNICODE | JSON_UNESCAPED_SLASHES ) . "\n";
        }
    }
}

$maintClass = WikiReaderRecords::class;
require_once RUN_MAINTENANCE_IF_MAIN;
