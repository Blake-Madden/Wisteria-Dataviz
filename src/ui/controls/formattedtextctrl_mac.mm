///////////////////////////////////////////////////////////////////////////////
// Name:        formattedtextctrl_mac.mm
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// Licence:     3-Clause BSD licence
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "formattedtextctrl_mac.h"

#ifdef __WXOSX__

#import <Cocoa/Cocoa.h>
#include <wx/osx/private.h>

// custom NSTextView subclass that provides headers and footers during printing
@interface TextViewForPrinting : NSTextView
{
@public
    NSMutableAttributedString* m_pageHeader;
    NSMutableAttributedString* m_pageFooter;
}

- (NSAttributedString*)pageHeader;
- (NSAttributedString*)pageFooter;

@end

@implementation TextViewForPrinting

- (instancetype)initWithFrame:(NSRect)frameRect
{
    self = [super initWithFrame:frameRect];
    if (self)
        {
        m_pageHeader = nil;
        m_pageFooter = nil;
        }
    return self;
}

- (void)dealloc
{
    [m_pageHeader release];
    [m_pageFooter release];
    [super dealloc];
}

- (NSFont*)headerFooterFont
{
    NSFont* font = [NSFont systemFontOfSize:12];
    return font;
}

- (NSAttributedString*)pageHeader
{
    if (m_pageHeader == nil)
        return nil;

    NSMutableString* formattedHeader =
        [[[NSMutableString alloc] initWithString:[m_pageHeader string]] autorelease];

    // replace @PN with current page number
    NSString* pageNumberStr =
        [NSString stringWithFormat:@"%lu",
            static_cast<unsigned long>([[NSPrintOperation currentOperation] currentPage])];
    [formattedHeader replaceOccurrencesOfString:@"@PN"
                                     withString:pageNumberStr
                                        options:0
                                          range:NSMakeRange(0, [formattedHeader length])];

    // replace @PC with total page count
    NSRange pageRange = [[NSPrintOperation currentOperation] pageRange];
    NSString* pageCountStr =
        [NSString stringWithFormat:@"%lu", static_cast<unsigned long>(pageRange.length)];
    [formattedHeader replaceOccurrencesOfString:@"@PC"
                                     withString:pageCountStr
                                        options:0
                                          range:NSMakeRange(0, [formattedHeader length])];

    NSMutableAttributedString* attributedFormattedHeader =
        [[[NSMutableAttributedString alloc] initWithString:formattedHeader] autorelease];
    [attributedFormattedHeader addAttribute:NSFontAttributeName
                                      value:[self headerFooterFont]
                                      range:NSMakeRange(0, [attributedFormattedHeader length])];

    // handle tabs as center/right
    CGFloat pageWidth = [[NSPrintOperation currentOperation].printInfo paperSize].width;
    NSMutableParagraphStyle* style = [[[NSMutableParagraphStyle alloc] init] autorelease];

    // center tab at half width, right tab near right margin
    NSTextTab* centerTab = [[NSTextTab alloc] initWithTextAlignment:NSTextAlignmentCenter
                                                         location:pageWidth/2
                                                           options:@{}];
    NSTextTab* rightTab = [[NSTextTab alloc] initWithTextAlignment:NSTextAlignmentRight
                                                        location:pageWidth-36  // 36 pt right margin
                                                          options:@{}];
    [style setTabStops:@[centerTab, rightTab]];

    [attributedFormattedHeader addAttribute:NSParagraphStyleAttributeName
                                      value:style
                                      range:NSMakeRange(0, [attributedFormattedHeader length])];

    return attributedFormattedHeader;
}

- (NSAttributedString*)pageFooter
{
    if (m_pageFooter == nil)
        return nil;

    NSMutableString* formattedFooter =
        [[[NSMutableString alloc] initWithString:[m_pageFooter string]] autorelease];

    // replace @PN with current page number
    NSString* pageNumberStr =
        [NSString stringWithFormat:@"%lu",
            static_cast<unsigned long>([[NSPrintOperation currentOperation] currentPage])];
    [formattedFooter replaceOccurrencesOfString:@"@PN"
                                     withString:pageNumberStr
                                        options:0
                                          range:NSMakeRange(0, [formattedFooter length])];

    // replace @PC with total page count
    NSRange pageRange = [[NSPrintOperation currentOperation] pageRange];
    NSString* pageCountStr =
        [NSString stringWithFormat:@"%lu", static_cast<unsigned long>(pageRange.length)];
    [formattedFooter replaceOccurrencesOfString:@"@PC"
                                     withString:pageCountStr
                                        options:0
                                          range:NSMakeRange(0, [formattedFooter length])];

    NSMutableAttributedString* attributedFormattedFooter =
        [[[NSMutableAttributedString alloc] initWithString:formattedFooter] autorelease];
    [attributedFormattedFooter addAttribute:NSFontAttributeName
                                      value:[self headerFooterFont]
                                      range:NSMakeRange(0, [attributedFormattedFooter length])];

    // handle tabs as center/right
    CGFloat pageWidth = [[NSPrintOperation currentOperation].printInfo paperSize].width;
    NSMutableParagraphStyle* style = [[[NSMutableParagraphStyle alloc] init] autorelease];

    NSTextTab* centerTab = [[NSTextTab alloc] initWithTextAlignment:NSTextAlignmentCenter
                                                         location:pageWidth/2
                                                           options:@{}];
    NSTextTab* rightTab = [[NSTextTab alloc] initWithTextAlignment:NSTextAlignmentRight
                                                        location:pageWidth-36
                                                          options:@{}];
    [style setTabStops:@[centerTab, rightTab]];

    [attributedFormattedFooter addAttribute:NSParagraphStyleAttributeName
                                      value:style
                                      range:NSMakeRange(0, [attributedFormattedFooter length])];

    return attributedFormattedFooter;
}

@end

namespace Wisteria::UI
    {
    //------------------------------------------------------------------
    void macOSPrintRTF(const wxString& rtfContent,
                       const wxSize& paperSize,
                       int orientation,
                       const wxString& header,
                       const wxString& footer)
        {
        @autoreleasepool
            {
            // set up print info
            NSPrintInfo* sharedInfo = [NSPrintInfo sharedPrintInfo];
            NSMutableDictionary* sharedDict = [sharedInfo dictionary];
            NSMutableDictionary* printInfoDict =
                [NSMutableDictionary dictionaryWithDictionary:sharedDict];

            // enable header and footer functionality
            [printInfoDict setValue:[NSNumber numberWithBool:YES]
                             forKey:NSPrintHeaderAndFooter];

            NSPrintInfo* printInfo =
                [[[NSPrintInfo alloc] initWithDictionary:printInfoDict] autorelease];

            // set margins (36 points = 0.5 inch)
            [printInfo setTopMargin:36];
            [printInfo setBottomMargin:36];
            [printInfo setLeftMargin:36];
            [printInfo setRightMargin:36];

            // set paper size and orientation
            NSSize nsPaperSize = {
                static_cast<CGFloat>(paperSize.GetWidth()),
                static_cast<CGFloat>(paperSize.GetHeight())
            };
            [printInfo setPaperSize:nsPaperSize];
            [printInfo setHorizontalPagination:NSPrintingPaginationModeFit];
            [printInfo setVerticalPagination:NSPrintingPaginationModeAutomatic];
            [printInfo setOrientation:(orientation == wxPORTRAIT) ?
                NSPaperOrientationPortrait : NSPaperOrientationLandscape];
            [printInfo setVerticallyCentered:NO];

            // create print view
            TextViewForPrinting* printView =
                [[[TextViewForPrinting alloc]
                    initWithFrame:NSMakeRect(0.0, 0.0, nsPaperSize.width, nsPaperSize.height)]
                    autorelease];

            // set up headers and footers
            if (!header.empty())
                {
                printView->m_pageHeader =
                    [[NSMutableAttributedString alloc]
                        initWithString:wxCFStringRef(header).AsNSString()];
                }
            else
                {
                printView->m_pageHeader = nil;
                }

            if (!footer.empty())
                {
                printView->m_pageFooter =
                    [[NSMutableAttributedString alloc]
                        initWithString:wxCFStringRef(footer).AsNSString()];
                }
            else
                {
                printView->m_pageFooter = nil;
                }

            // convert RTF string to NSData and load into the text view
            // RTF should be 7-bit ASCII
            const wxScopedCharBuffer rtfBuffer = rtfContent.ToUTF8();
            NSData* rtfData = [NSData dataWithBytes:rtfBuffer.data()
                                             length:rtfBuffer.length()];

            NSRange printViewRange = NSMakeRange(0, [[printView textStorage] length]);
            [printView replaceCharactersInRange:printViewRange withRTF:rtfData];

            // run the print operation
            NSPrintOperation* printOp =
                [NSPrintOperation printOperationWithView:printView printInfo:printInfo];
            [printOp runOperation];
            // printView is autoreleased; its dealloc will clean up m_pageHeader and m_pageFooter
            }
        }
    } // namespace Wisteria::UI

#endif // __WXOSX__
