#include <qmsgbox.h>

#include "status.h"
#include "defines.h"

#include <stdlib.h>

#include <qpainter.h>
#include <qpixmap.h>
#include <qfile.h>
#include <qfileinf.h>
#include <qtstream.h>

#include <kconfig.h>
#include <kapp.h>

#include "status.moc"


KStatus::KStatus ( QWidget *parent, const char *name )
: QWidget( parent, name )
{
	frame = new QFrame(this);
	frame->setFrameStyle( QFrame::Box | QFrame::Raised );
	frame->setLineWidth(2);
	frame->setMidLineWidth(2);
	
	field = new Field(this);
	
	connect( this , SIGNAL(newField(int, int, int)),
			 field, SLOT(Start(int, int, int)) );
	connect( this,  SIGNAL(stopField()),
		     field, SLOT(Stop()) );
	connect( this, SIGNAL(pause()),
			 field,  SLOT(pause()) );
  
	connect( field, SIGNAL(changeCase(int,int)), SLOT(changeCase(int,int)));
	connect( field, SIGNAL(updateStatus()), SLOT(update()));
	connect( field, SIGNAL(endGame(int)), SLOT(endGame(int)));
  
	smiley = new QPushButton(this);
	connect( smiley, SIGNAL(clicked()), SLOT(restartGame()));
	smiley->installEventFilter(parent);
  
	QPainter pt;
	
	s_ok = new QPixmap(25,25);
	createSmileyPixmap(s_ok,&pt);
	pt.drawPoint(8,14); pt.drawPoint(16,14);
	pt.drawLine(9,15,15,15);
	pt.end();
	s_stress = new QPixmap(25,25);
	createSmileyPixmap(s_stress,&pt);
	pt.drawPoint(12,13);
	pt.drawLine(11,14,11,15); pt.drawLine(13,14,13,15);
	pt.drawPoint(12,16);
	pt.end();
	s_happy = new QPixmap(25,25);
	createSmileyPixmap(s_happy,&pt);
	pt.drawPoint(7,14); pt.drawPoint(17,14);
	pt.drawPoint(8,15); pt.drawPoint(16,15);
	pt.drawPoint(9,16); pt.drawPoint(15,16);
	pt.drawLine(10,17,14,17);
	pt.end();
	s_ohno = new QPixmap(25,25);
	createSmileyPixmap(s_ohno,&pt);  pt.drawPoint(12,11);
	pt.drawLine(10,13,14,13);
	pt.drawLine(9,14,9,17); pt.drawLine(15,14,15,17);
	pt.drawLine(10,18,14,18);
	pt.end();
  
	dg = new DigitalClock(this);
	dg->installEventFilter(parent);
	
	left = new QLCDNumber(this);
	left->setFrameStyle(  QFrame::Panel | QFrame::Sunken );
	left->installEventFilter(parent);
	
	QFont f("Times", 14, QFont::Bold);
	
	mesg = new QLabel(this);
	mesg->setAlignment( AlignCenter );
	mesg->setFrameStyle(  QFrame::Panel | QFrame::Sunken );
	mesg->setFont( f );
	mesg->installEventFilter(parent);
	
	connect( this, SIGNAL(exleft(const char *)),
			 left, SLOT(display(const char *)) );
	connect( this, SIGNAL(freezeTimer()),
			 dg,   SLOT(freeze()) );
	connect( this, SIGNAL(getTime(int *, int *)),
		 	 dg,   SLOT(getTime(int *, int *)) );
	
	connect( field, SIGNAL(startTimer()),
			 dg,    SLOT(start()) );
	connect( field, SIGNAL(freezeTimer()),
			 dg,    SLOT(freeze()) );
	connect( this,  SIGNAL(zeroTimer()),
			 dg,    SLOT(zero()) );
	connect( this,  SIGNAL(startTimer()),
		     dg,    SLOT(start()) );
	connect( field, SIGNAL(updateSmiley(int)),
			 this,  SLOT(updateSmiley(int)) );
	connect( this,  SIGNAL(setUMark(int)),
			 field, SLOT(setUMark(int)) );
	
	/* configuration & highscore initialisation */
	kconf = kapp->getConfig();
	isConfigWritable =
		(kapp->getConfigState()==KApplication::APPCONFIG_READWRITE);
	
	/* if the entries do not exist : create them */
	kconf->setGroup(OP_GRP);
	if ( !kconf->hasKey(OP_UMARK_KEY) )
		kconf->writeEntry(OP_UMARK_KEY, 0);
	for (int i=0; i<3; i++) {
		kconf->setGroup(HS_GRP[i]);
		if ( !kconf->hasKey(HS_NAME_KEY) )
			kconf->writeEntry(HS_NAME_KEY, "Anonymous");        
		if ( !kconf->hasKey(HS_MIN_KEY) )
			kconf->writeEntry(HS_MIN_KEY, 59);
		if ( !kconf->hasKey(HS_SEC_KEY) )
			kconf->writeEntry(HS_SEC_KEY, 59);    
	}
	
	/* read configuration */
	kconf->setGroup(OP_GRP);
	setUMark(kconf->readNumEntry(OP_UMARK_KEY));
}


void KStatus::createSmileyPixmap(QPixmap *pm, QPainter *pt)
{
	pm->fill(yellow);
	pt->begin(pm);
	pt->setPen(black);
	pt->drawLine(9,3,15,3);
	pt->drawLine(7,4,8,4); pt->drawLine(16,4,17,4);
	pt->drawPoint(6,5); pt->drawPoint(18,5);
	pt->drawPoint(5,6); pt->drawPoint(19,6);
	pt->drawLine(4,7,4,8); pt->drawLine(20,7,20,8);
	pt->drawLine(8,7,9,7); pt->drawLine(15,7,16,7);
	pt->drawLine(3,9,3,14); pt->drawLine(21,9,21,14);
	pt->drawPoint(12,10);
	pt->drawLine(4,15,4,17); pt->drawLine(20,15,20,17);
	pt->drawPoint(5,18); pt->drawPoint(19,18);
	pt->drawLine(6,19,7,19); pt->drawLine(17,19,18,19);
	pt->drawLine(8,20,9,20); pt->drawLine(15,20,16,20);
	pt->drawLine(10,21,14,21);
}


void KStatus::adjustSize()
{ 
	int dec_w, dec_h, dec_hs;

	dec_w  = (width() - 2*LCD_W - SMILEY_W)/4;
	dec_h  = (STATUS_H - LCD_H)/2;
	dec_hs = (STATUS_H - SMILEY_H)/2;
	
	left->setGeometry( dec_w, dec_h, LCD_W, LCD_H );
	mesg->setGeometry( dec_w, dec_h, LCD_W, LCD_H );
	smiley->setGeometry( 2*dec_w + LCD_W, dec_hs, SMILEY_W, SMILEY_H );
	dg->setGeometry( 3*dec_w + LCD_W + SMILEY_W, dec_h, LCD_W, LCD_H );

	
	dec_w = (width() - 2*FRAME_W - nb_width*CASE_W)/2;
	dec_h = (height() - STATUS_H - 2*FRAME_W - nb_height*CASE_W)/2;
	
	frame->setGeometry( dec_w, STATUS_H + dec_h,
					    nb_width*CASE_W + 2*FRAME_W,
					    nb_height*CASE_W + 2*FRAME_W );
	field->setGeometry( FRAME_W + dec_w, STATUS_H + FRAME_W + dec_h ,
					    nb_width*CASE_W, nb_height*CASE_W );
}


void KStatus::restartGame()
{
	uncovered = 0; uncertain = 0; marked = 0;
	
	/* hide the message label */
	mesg->hide();
	
	update();
	updateSmiley(OK);
	
	emit zeroTimer();
	
	emit newField( nb_width, nb_height, nb_mines);
}


void KStatus::newGame(int nb_w, int nb_h, int nb_m)
{
	nb_width  = nb_w;
	nb_height = nb_h;
	nb_mines  = nb_m;
	
	adjustSize();
  
	restartGame();
}


void KStatus::changeCase( int case_mode, int inc)
{
	switch(case_mode) {
	 case UNCOVERED : uncovered += inc; break;
	 case UNCERTAIN : uncertain += inc; break;
	 case MARKED    : marked    += inc; break;
	}
}


void KStatus::update()
{
	static char perc[5];
	sprintf(perc,"%d", nb_mines - marked);
	emit exleft(perc);
	
	if (uncovered==(nb_width*nb_height - nb_mines))
		endGame(TRUE);
}


void KStatus::updateSmiley(int mood)
{
	switch (mood) {
	 case OK      : smiley->setPixmap(*s_ok); break;
	 case STRESS  : smiley->setPixmap(*s_stress); break;
	 case HAPPY   : smiley->setPixmap(*s_happy); break;
	 case UNHAPPY : smiley->setPixmap(*s_ohno); break;
	}
}


void KStatus::endGame(int win)
{
	int t_sec, t_min;
	
	emit stopField();
	emit freezeTimer();
	
	if (win) {
		emit updateSmiley(HAPPY);
		exmesg("You did it !");
		
		emit getTime(&t_sec, &t_min);
		
		if (nb_width==MODES[0][0] && nb_height==MODES[0][1]
			&& nb_mines==MODES[0][2])
			setHighScore(t_sec,t_min,EASY);
		else if (nb_width==MODES[1][0] && nb_height==MODES[1][1]
				 && nb_mines==MODES[1][2])
			setHighScore(t_sec,t_min,NORMAL);
	  else if (nb_width==MODES[2][0] && nb_height==MODES[2][1]
			   && nb_mines==MODES[2][2])
			setHighScore(t_sec,t_min,EXPERT);
	} else {
		emit updateSmiley(UNHAPPY);
		exmesg("Try again ...");
	}
}


void KStatus::getNumbers(int *nb_w, int *nb_h, int *nb_m)
{
	*nb_w = nb_width; *nb_h = nb_height; *nb_m = nb_mines;
}

void KStatus::options()
{
	Options::Options(this);

	kconf->setGroup(OP_GRP);
	emit setUMark(kconf->readNumEntry(OP_UMARK_KEY));
}

void KStatus::exmesg(const char *str)
{ 
	mesg->show();
	mesg->setText(str);
}

void KStatus::showHighScores()
{
	WHighScores whs(TRUE, 0, 0, 0, this);
}

void KStatus::setHighScore(int sec, int min, int mode)
{
	if ( isConfigWritable ) {
		WHighScores whs(FALSE, sec, min, mode, this);
		
		/* save the new score (in the file to be sure it won't be lost) */
		if (isConfigWritable)
			kconf->sync();
	} else
		errorShow("Highscore file is not writable !");
} 

void KStatus::pauseGame()
{
	emit pause();
}

void KStatus::errorShow(QString msg)
{
	QMessageBox ab;   
	ab.setCaption("kmines : Error"); 
	ab.setText(msg);   
	ab.setButtonText("Ok");        
	ab.exec();
}
