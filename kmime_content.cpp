/*
    kmime_content.cpp

    KMime, the KDE internet mail/usenet news message library.
    Copyright (c) 2001 the KMime authors.
    See file AUTHORS for details

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software Foundation,
    Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, US
*/
#include "kmime_content.h"
#include "kmime_parsers.h"

#include <kcharsets.h>
#include <kmdcodec.h>
#include <kglobal.h>
#include <klocale.h>
#include <kdebug.h>

#include <qtextcodec.h>

using namespace KMime;

namespace KMime {

Content::Content()
 : c_ontents(0), h_eaders(0), f_orceDefaultCS(false)
{
  d_efaultCS = cachedCharset("ISO-8859-1");
}


Content::Content(const QCString &h, const QCString &b)
 : c_ontents(0), h_eaders(0), f_orceDefaultCS(false)
{
  d_efaultCS = cachedCharset("ISO-8859-1");
  h_ead=h.copy();
  b_ody=b.copy();
}


Content::~Content()
{
  delete c_ontents;
  delete h_eaders;
}


void Content::setContent(QStrList *l)
{
  //qDebug("Content::setContent(QStrList *l) : start");
  h_ead.resize(0);
  b_ody.resize(0);

  //usage of textstreams is much faster than simply appending the strings
  QTextStream hts(h_ead, IO_WriteOnly),
              bts(b_ody, IO_WriteOnly);
  hts.setEncoding(QTextStream::Latin1);
  bts.setEncoding(QTextStream::Latin1);

  bool isHead=true;
  for(char *line=l->first(); line; line=l->next()) {
    if(isHead && line[0]=='\0') {
      isHead=false;
      continue;
    }
    if(isHead)
      hts << line << "\n";
    else
      bts << line << "\n";
  }

  //terminate strings
  hts << '\0';
  bts << '\0';

  //qDebug("Content::setContent(QStrList *l) : finished");
}


void Content::setContent(const QCString &s)
{
  int pos=s.find("\n\n", 0);
  if(pos>-1) {
    h_ead=s.left(++pos);  //header *must* end with "\n" !!
    b_ody=s.mid(++pos, s.length()-pos);
  }
  else
    h_ead=s;
}


//parse the message, split multiple parts
void Content::parse()
{
  //qDebug("void Content::parse() : start");
  delete h_eaders;
  h_eaders=0;
  
  // check this part has already been partioned into subparts.
  // if this is the case, we will not try to reparse the body
  // of this part.
  if ((b_ody.size() == 0) && (c_ontents != 0) && !c_ontents->isEmpty()) {
    // reparse all sub parts
    for(Content *c=c_ontents->first(); c; c=c_ontents->next())
      c->parse();
    return;
  }    
  
  delete c_ontents;
  c_ontents=0;

  Headers::ContentType *ct=contentType();
  QCString tmp;
  Content *c;
  Headers::contentCategory cat;

  // just "text" as mimetype is suspicious, perhaps this article was
  // generated by broken software, better check for uuencoded binaries
  if (ct->mimeType()=="text")
    ct->setMimeType("invalid/invalid");

  if(ct->isText())
    return; //nothing to do

  if(ct->isMultipart()) {   //this is a multipart message
    tmp=ct->boundary(); //get boundary-parameter

    if(!tmp.isEmpty()) {
      Parser::MultiPart mpp(b_ody, tmp);
      if(mpp.parse()) { //at least one part found

        c_ontents=new List();
        c_ontents->setAutoDelete(true);

        if(ct->isSubtype("alternative")) //examine category for the sub-parts
          cat=Headers::CCalternativePart;
        else
          cat=Headers::CCmixedPart;  //default to "mixed"

        QCStringList parts=mpp.parts();
        QCStringList::Iterator it;
        for(it=parts.begin(); it!=parts.end(); ++it) { //create a new Content for every part
          c=new Content();
          c->setContent(*it);
          c->parse();
          c->contentType()->setCategory(cat); //set category of the sub-part
          c_ontents->append(c);
          //qDebug("part:\n%s\n\n%s", c->h_ead.data(), c->b_ody.left(100).data());
        }

        //the whole content is now split into single parts, so it's safe delete the message-body
        b_ody.resize(0);
      }
      else { //sh*t, the parsing failed so we have to treat the message as "text/plain" instead
        ct->setMimeType("text/plain");
        ct->setCharset("US-ASCII");
      }
    }
  }
  else if (ct->mimeType()=="invalid/invalid") { //non-mime body => check for uuencoded content
    Parser::UUEncoded uup(b_ody, rawHeader("Subject"));

    if(uup.parse()) { // yep, it is uuencoded

      if(uup.isPartial()) {  // this seems to be only a part of the message so we treat it as "message/partial"
        ct->setMimeType("message/partial");
        //ct->setId(uniqueString()); not needed yet
        ct->setPartialParams(uup.partialCount(), uup.partialNumber());
        contentTransferEncoding()->setCte(Headers::CE7Bit);
      }
      else { //it's a complete message => treat as "multipart/mixed"
        //the whole content is now split into single parts, so it's safe to delete the message-body
        b_ody.resize(0);

        //binary parts
        for (unsigned int i=0;i<uup.binaryParts().count();i++) {
          c=new Content();
          //generate content with mime-compliant headers
          tmp="Content-Type: ";
          tmp += uup.mimeTypes().at(i);
          tmp += "; name=\"";
          tmp += uup.filenames().at(i);
          tmp += "\"\nContent-Transfer-Encoding: x-uuencode\nContent-Disposition: attachment; filename=\"";
          tmp += uup.filenames().at(i);
          tmp += "\"\n\n";
          tmp += uup.binaryParts().at(i);
          c->setContent(tmp);
          addContent(c);
        }

        if(c_ontents && c_ontents->first()) { //readd the plain text before the uuencoded part
          c_ontents->first()->setContent("Content-Type: text/plain\nContent-Transfer-Encoding: 7Bit\n\n"+uup.textPart());
          c_ontents->first()->contentType()->setMimeType("text/plain");
        }
      }
    }
    else { //no, this doesn't look like uuencoded stuff => we treat it as "text/plain"
      ct->setMimeType("text/plain");
    }
  }

  //qDebug("void Content::parse() : finished");
}


void Content::assemble()
{
  QCString newHead="";

  //Content-Type
  newHead+=contentType()->as7BitString()+"\n";

  //Content-Transfer-Encoding
  newHead+=contentTransferEncoding()->as7BitString()+"\n";

  //Content-Description
  Headers::Base *h=contentDescription(false);
  if(h)
    newHead+=h->as7BitString()+"\n";

  //Content-Disposition
  h=contentDisposition(false);
  if(h)
    newHead+=h->as7BitString()+"\n";

  h_ead=newHead;
}


void Content::clear()
{
  delete h_eaders;
  h_eaders=0;
  delete c_ontents;
  c_ontents=0;
  h_ead.resize(0);
  b_ody.resize(0);
}


QCString Content::encodedContent(bool useCrLf)
{
  QCString e;

  // hack to convert articles with uuencoded binaries into
  // proper mime-compliant articles
  if(c_ontents && !c_ontents->isEmpty()) {
    bool convertFromUunec=false;

    // reencode uuencoded binaries...
    for(Content *c=c_ontents->first(); c; c=c_ontents->next()) {
      if (c->contentTransferEncoding(true)->cte()==Headers::CEuuenc) {
        convertFromUunec=true;
        c->b_ody = KCodecs::base64Encode(c->decodedContent(), true);
        c->b_ody.append("\n");
        c->contentTransferEncoding(true)->setCte(Headers::CEbase64);
        c->contentTransferEncoding(true)->setDecoded(false);
        c->removeHeader("Content-Description");
        c->assemble();
      }
    }

    // add proper mime headers...
    if (convertFromUunec) {
      h_ead.replace(QRegExp("MIME-Version: .*\\n"),"");
      h_ead.replace(QRegExp("Content-Type: .*\\n"),"");
      h_ead.replace(QRegExp("Content-Transfer-Encoding: .*\\n"),"");
      h_ead+="MIME-Version: 1.0\n";
      h_ead+=contentType(true)->as7BitString()+"\n";
      h_ead+=contentTransferEncoding(true)->as7BitString()+"\n";
    }
  }

  //head
  e=h_ead.copy();
  e+="\n";

  //body
  if(!b_ody.isEmpty()) { //this message contains only one part
    Headers::CTEncoding *enc=contentTransferEncoding();

    if(enc->needToEncode()) {
      if(enc->cte()==Headers::CEquPr) {
        QByteArray temp(b_ody.length());
        memcpy(temp.data(), b_ody.data(), b_ody.length());
        e+=KCodecs::quotedPrintableEncode(temp, false);
      } else {
        e+=KCodecs::base64Encode(b_ody, true);
        e+="\n";
      }
    }
    else
      e+=b_ody;
  }
  else if(c_ontents && !c_ontents->isEmpty()) { //this is a multipart message
    Headers::ContentType *ct=contentType();
    QCString boundary="--"+ct->boundary();

    //add all (encoded) contents separated by boundaries
    for(Content *c=c_ontents->first(); c; c=c_ontents->next()) {
      e+=boundary+"\n";
      e+=c->encodedContent(false);  // don't convert LFs here, we do that later!!!!!
    }
    //finally append the closing boundary
    e+=boundary+"--\n";
  };

  if(useCrLf)
    return LFtoCRLF(e);
  else
    return e;
}


QByteArray Content::decodedContent()
{
  QByteArray temp, ret;
  Headers::CTEncoding *ec=contentTransferEncoding();
  bool removeTrailingNewline=false;

  if (b_ody.length()==0)
    return ret;

  temp.resize(b_ody.length());
  memcpy(temp.data(), b_ody.data(), b_ody.length());

  if(ec->decoded()) {
    ret = temp;
    removeTrailingNewline=true;
  } else {
    switch(ec->cte()) {
      case Headers::CEbase64 :
        KCodecs::base64Decode(temp, ret);
      break;
      case Headers::CEquPr :
        ret = KCodecs::quotedPrintableDecode(b_ody);
        ret.resize(ret.size()-1);  // remove null-char
        removeTrailingNewline=true;
      break;
      case Headers::CEuuenc :
        KCodecs::uudecode(temp, ret);
      break;
      default :
        ret = temp;
        removeTrailingNewline=true;
    }
  }

  if (removeTrailingNewline && (ret.size()>0) && (ret[ret.size()-1] == '\n'))
    ret.resize(ret.size()-1);

  return ret;
}


void Content::decodedText(QString &s, bool trimText,
			  bool removeTrailingNewlines)
{
  if(!decodeText()) //this is not a text content !!
    return;

  bool ok=true;
  QTextCodec *codec=KGlobal::charsets()->codecForName(contentType()->charset(),ok);

  s=codec->toUnicode(b_ody.data(), b_ody.length());

  if (trimText && removeTrailingNewlines) {
    int i;
    for (i=s.length()-1; i>=0; i--)
      if (!s[i].isSpace())
        break;
    s.truncate(i+1);
  } else {
    if (s.right(1)=="\n")
      s.truncate(s.length()-1);    // remove trailing new-line
  }
}


void Content::decodedText(QStringList &l, bool trimText,
			  bool removeTrailingNewlines)
{
  if(!decodeText()) //this is not a text content !!
    return;

  QString unicode;
  bool ok=true;

  QTextCodec *codec=KGlobal::charsets()->codecForName(contentType()->charset(),ok);

  unicode=codec->toUnicode(b_ody.data(), b_ody.length());

  if (trimText && removeTrailingNewlines) {
    int i;
    for (i=unicode.length()-1; i>=0; i--)
      if (!unicode[i].isSpace())
        break;
    unicode.truncate(i+1);
  } else {
    if (unicode.right(1)=="\n")
      unicode.truncate(unicode.length()-1);    // remove trailing new-line
  }

  l=QStringList::split('\n', unicode, true); //split the string at linebreaks
}


void Content::fromUnicodeString(const QString &s)
{
  bool ok=true;
  QTextCodec *codec=KGlobal::charsets()->codecForName(contentType()->charset(),ok);

  if(!ok) { // no suitable codec found => try local settings and hope the best ;-)
    codec=KGlobal::locale()->codecForEncoding();
#if 0 // dependance on KNode
    QCString chset=knGlobals.cfgManager->postNewsTechnical()->findComposerCharset(KGlobal::locale()->charset().latin1());
    if (chset.isEmpty())
#endif
    QCString chset=KGlobal::locale()->encoding();
    contentType()->setCharset(chset);
  }

  b_ody=codec->fromUnicode(s);
  contentTransferEncoding()->setDecoded(true); //text is always decoded
}


Content* Content::textContent()
{
  Content *ret=0;

  //return the first content with mimetype=text/*
  if(contentType()->isText())
    ret=this;
  else if(c_ontents)
    for(Content *c=c_ontents->first(); c; c=c_ontents->next())
      if( (ret=c->textContent())!=0 )
        break;

  return ret;
}


void Content::attachments(Content::List *dst, bool incAlternatives)
{
  dst->setAutoDelete(false); //don't delete the contents

  if(!c_ontents)
    dst->append(this);
  else {
    for(Content *c=c_ontents->first(); c; c=c_ontents->next()) {
      if( !incAlternatives && c->contentType()->category()==Headers::CCalternativePart)
        continue;
      else
        c->attachments(dst, incAlternatives);
    }
  }

  if(type()!=ATmimeContent) { // this is the toplevel article
    Content *text=textContent();
    if(text)
      dst->removeRef(text);
  }
}


void Content::addContent(Content *c, bool prepend)
{
  if(!c_ontents) { // this message is not multipart yet
    c_ontents=new List();
    c_ontents->setAutoDelete(true);

    // first we convert the body to a content
    Content *main=new Content();

    //the Mime-Headers are needed, so we move them to the new content
    if(h_eaders) {

      main->h_eaders=new Headers::Base::List();
      main->h_eaders->setAutoDelete(true);

      Headers::Base::List srcHdrs=(*h_eaders);
      srcHdrs.setAutoDelete(false);
      int idx=0;
      for(Headers::Base *h=srcHdrs.first(); h; h=srcHdrs.next()) {
        if(h->isMimeHeader()) {
          //remove from this content
          idx=h_eaders->findRef(h);
          h_eaders->take(idx);
          //append to new content
          main->h_eaders->append(h);
        }
      }
    }

    //"main" is now part of a multipart/mixed message
    main->contentType()->setCategory(Headers::CCmixedPart);

    //the head of "main" is empty, so we assemble it
    main->assemble();

    //now we can copy the body and append the new content;
    main->b_ody=b_ody.copy();
    c_ontents->append(main);
    b_ody.resize(0); //not longer needed


    //finally we have to convert this article to "multipart/mixed"
    Headers::ContentType *ct=contentType();
    ct->setMimeType("multipart/mixed");
    ct->setBoundary(multiPartBoundary());
    ct->setCategory(Headers::CCcontainer);
    contentTransferEncoding()->clear();  // 7Bit, decoded

  }
  //here we actually add the content
  if(prepend)
    c_ontents->insert(0, c);
  else
    c_ontents->append(c);
}


void Content::removeContent(Content *c, bool del)
{
  if(!c_ontents) // what the ..
    return;

  int idx=0;
  if(del)
    c_ontents->removeRef(c);
  else {
    idx=c_ontents->findRef(c);
    c_ontents->take(idx);
  }

  //only one content left => turn this message in a single-part
  if(c_ontents->count()==1) {
    Content *main=c_ontents->first();

    //first we have to move the mime-headers
    if(main->h_eaders) {
      if(!h_eaders) {
        h_eaders=new Headers::Base::List();
        h_eaders->setAutoDelete(true);
      }

      Headers::Base::List mainHdrs=(*(main->h_eaders));
      mainHdrs.setAutoDelete(false);

      for(Headers::Base *h=mainHdrs.first(); h; h=mainHdrs.next()) {
        if(h->isMimeHeader()) {
          removeHeader(h->type()); //remove the old header first
          h_eaders->append(h); //now append the new one
          idx=main->h_eaders->findRef(h);
          main->h_eaders->take(idx); //remove from the old content
          kdDebug(5003) << "Content::removeContent(Content *c, bool del) : mime-header moved: "
                        << h->as7BitString() << endl;
        }
      }
    }

    //now we can copy the body
    b_ody=main->b_ody.copy();

    //finally we can delete the content list
    delete c_ontents;
    c_ontents=0;
  }
}


void Content::changeEncoding(Headers::contentEncoding e)
{
  Headers::CTEncoding *enc=contentTransferEncoding();
  if(enc->cte()==e) //nothing to do
    return;

  if(decodeText())
    enc->setCte(e); // text is not encoded until it's sent or saved so we just set the new encoding
  else { // this content contains non textual data, that has to be re-encoded

    if(e!=Headers::CEbase64) {
      //kdWarning(5003) << "Content::changeEncoding() : non textual data and encoding != base64 - this should not happen\n => forcing base64" << endl;
      e=Headers::CEbase64;
    }

    if(enc->cte()!=e) { // ok, we reencode the content using base64
      b_ody = KCodecs::base64Encode(decodedContent(), true);
      b_ody.append("\n");
      enc->setCte(e); //set encoding
      enc->setDecoded(false);
    }
  }
}


void Content::toStream(QTextStream &ts, bool scrambleFromLines)
{
  QCString ret=encodedContent(false);

  if (scrambleFromLines)
    ret.replace(QRegExp("\\n\\nFrom "), "\n\n>From ");

  ts << ret;
}


Headers::Generic*  Content::getNextHeader(QCString &head)
{
  int pos1=-1, pos2=0, len=head.length()-1;
  bool folded(false);
  Headers::Generic *header=0;

  pos1 = head.find(": ");

  if (pos1>-1) {    //there is another header
    pos2=pos1+=2; //skip the name

    if (head[pos2]!='\n') {  // check if the header is not empty
      while(1) {
        pos2=head.find("\n", pos2+1);
        if(pos2==-1 || pos2==len || ( head[pos2+1]!=' ' && head[pos2+1]!='\t') ) //break if we reach the end of the string, honor folded lines
          break;
        else
          folded = true;
      }
    }

    if(pos2<0) pos2=len+1; //take the rest of the string

    if (!folded)
      header = new Headers::Generic(head.left(pos1-2), this, head.mid(pos1, pos2-pos1));
    else
      header = new Headers::Generic(head.left(pos1-2), this, head.mid(pos1, pos2-pos1).replace(QRegExp("\\s*\\n\\s*")," "));

    head.remove(0,pos2+1);
  }
  else {
    head = "";
  }

  return header;
}


Headers::Base* Content::getHeaderByType(const char *type)
{
  if(!type)
    return 0;

  Headers::Base *h=0;
  //first we check if the requested header is already cached
  if(h_eaders)
    for(h=h_eaders->first(); h; h=h_eaders->next())
      if(h->is(type)) return h; //found

  //now we look for it in the article head
  QCString raw=rawHeader(type);
  if(!raw.isEmpty()) { //ok, we found it
    //choose a suitable header class
    if(strcasecmp("Message-Id", type)==0)
      h=new Headers::MessageID(this, raw);
    else if(strcasecmp("Subject", type)==0)
      h=new Headers::Subject(this, raw);
    else if(strcasecmp("Date", type)==0)
      h=new Headers::Date(this, raw);
    else if(strcasecmp("From", type)==0)
      h=new Headers::From(this, raw);
    else if(strcasecmp("Organization", type)==0)
      h=new Headers::Organization(this, raw);
    else if(strcasecmp("Reply-To", type)==0)
      h=new Headers::ReplyTo(this, raw);
    else if(strcasecmp("Mail-Copies-To", type)==0)
      h=new Headers::MailCopiesTo(this, raw);
    else if(strcasecmp("To", type)==0)
      h=new Headers::To(this, raw);
    else if(strcasecmp("CC", type)==0)
      h=new Headers::CC(this, raw);
    else if(strcasecmp("BCC", type)==0)
      h=new Headers::BCC(this, raw);
    else if(strcasecmp("Newsgroups", type)==0)
      h=new Headers::Newsgroups(this, raw);
    else if(strcasecmp("Followup-To", type)==0)
      h=new Headers::FollowUpTo(this, raw);
    else if(strcasecmp("References", type)==0)
      h=new Headers::References(this, raw);
    else if(strcasecmp("Lines", type)==0)
      h=new Headers::Lines(this, raw);
    else if(strcasecmp("Content-Type", type)==0)
      h=new Headers::ContentType(this, raw);
    else if(strcasecmp("Content-Transfer-Encoding", type)==0)
      h=new Headers::CTEncoding(this, raw);
    else if(strcasecmp("Content-Disposition", type)==0)
      h=new Headers::CDisposition(this, raw);
    else if(strcasecmp("Content-Description", type)==0)
      h=new Headers::CDescription(this, raw);
    else
      h=new Headers::Generic(type, this, raw);

    if(!h_eaders) {
      h_eaders=new Headers::Base::List();
      h_eaders->setAutoDelete(true);
    }

    h_eaders->append(h);  //add to cache
    return h;
  }
  else
    return 0; //header not found
}


void Content::setHeader(Headers::Base *h)
{
  if(!h) return;
  removeHeader(h->type());
  if(!h_eaders) {
    h_eaders=new Headers::Base::List();
    h_eaders->setAutoDelete(true);
  }
  h_eaders->append(h);
}


bool Content::removeHeader(const char *type)
{
  if(h_eaders)
    for(Headers::Base *h=h_eaders->first(); h; h=h_eaders->next())
      if(h->is(type))
        return h_eaders->remove();

  return false;
}


int Content::size()
{
  int ret=b_ody.length();

  if(contentTransferEncoding()->cte()==Headers::CEbase64)
    return (ret*3/4); //base64 => 6 bit per byte

  return ret;
}


int Content::storageSize()
{
  int s=h_ead.length();

  if(!c_ontents)
    s+=b_ody.length();
  else {
    for(Content *c=c_ontents->first(); c; c=c_ontents->next())
      s+=c->storageSize();
  }

  return s;
}


int Content::lineCount()
{
  int ret=0;
  if(type()==ATmimeContent)
    ret+=h_ead.contains('\n');
  ret+=b_ody.contains('\n');

  if(c_ontents && !c_ontents->isEmpty())
    for(Content *c=c_ontents->first(); c; c=c_ontents->next())
      ret+=c->lineCount();

  return ret;
}


QCString Content::rawHeader(const char *name)
{
  return extractHeader(h_ead, name);
}


bool Content::decodeText()
{
  Headers::CTEncoding *enc=contentTransferEncoding();

  if(enc->decoded())
    return true; //nothing to do
  if(!contentType()->isText())
    return false; //non textual data cannot be decoded here => use decodedContent() instead

  switch(enc->cte()) {
    case Headers::CEbase64 :
      b_ody=KCodecs::base64Decode(b_ody);
      b_ody.append("\n");
    break;
    case Headers::CEquPr :
      b_ody=KCodecs::quotedPrintableDecode(b_ody);
    break;
    case Headers::CEuuenc :
      b_ody=KCodecs::uudecode(b_ody);
      b_ody.append("\n");
    break;
    default :
    break;
  }

  enc->setDecoded(true);
  return true;
}


void Content::setDefaultCharset(const QCString &cs)
{ 
  d_efaultCS = KMime::cachedCharset(cs); 
  
  if(c_ontents && !c_ontents->isEmpty())
    for(Content *c=c_ontents->first(); c; c=c_ontents->next())
      c->setDefaultCharset(cs);
      
  // reparse the part and its sub-parts in order
  // to clear cached header values
  parse();      
}


void Content::setForceDefaultCS(bool b)
{
  f_orceDefaultCS=b;
  
  if(c_ontents && !c_ontents->isEmpty())
    for(Content *c=c_ontents->first(); c; c=c_ontents->next())
      c->setForceDefaultCS(b);
  
  // reparse the part and its sub-parts in order
  // to clear cached header values    
  parse();
}


}; // namespace KMime
