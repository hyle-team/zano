import { IntToMoneyPipe } from './int-to-money.pipe';

describe('IntToMoneyPipe', () => {
  it('create an instance', () => {
    const pipe = new IntToMoneyPipe();
    expect(pipe).toBeTruthy();
  });
});
