import { Component, OnInit } from '@angular/core';
import { ActivatedRoute } from '@angular/router';

@Component({
  selector: 'app-typing-message',
  templateUrl: './typing-message.component.html',
  styleUrls: ['./typing-message.component.scss']
})
export class TypingMessageComponent implements OnInit {

  messagesId: number;
  private subMessages: any;

  constructor(private route: ActivatedRoute) {
    this.route.params.subscribe( params => console.log(params) );
  }

  ngOnInit() {

  }

}
